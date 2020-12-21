#!/usr/bin/env python

import numpy as np
import pandas as pd

import torch
import torchvision
from torchvision import datasets, transforms
from torch import nn, optim
from torch.optim.lr_scheduler import StepLR
from torch.nn import functional as F
from torch.utils.data import DataLoader, sampler, random_split

import matplotlib

import matplotlib.pyplot as plt
import seaborn as sns

from torchvision import models
from torch.utils.tensorboard import SummaryWriter

from PIL import Image

from ignite.engine import (
    Engine,
    Events,
    create_supervised_trainer,
    create_supervised_evaluator,
)
from ignite.handlers import ModelCheckpoint, EarlyStopping
from ignite.metrics import Accuracy, Loss, RunningAverage, ConfusionMatrix
from ignite.contrib.handlers import ProgressBar

import wandb
from darknet19 import darknet as darknet


WORKLOAD = "brickfinder"
LOG_DIR = "/home/mdw/tensorboard/"
DATASET_DIR = "/home/mdw/datasets/brick-dataset/images/"
CHECKPOINT_DIR = "/home/mdw/checkpoints/"
MAX_EPOCHS = 100
INITIAL_LR = 0.01
MOMENTUM = 0.9
GAMMA = 0.1

wandb.init(project=WORKLOAD)
config = wandb.config
config.max_epochs = MAX_EPOCHS
config.initial_lr = INITIAL_LR
config.momentum = MOMENTUM
config.gamma = GAMMA


def get_data_loaders(data_dir, batch_size):
    transform = transforms.Compose(
        [transforms.Resize(255), transforms.CenterCrop(224), transforms.ToTensor()]
    )
    all_data = datasets.ImageFolder(data_dir, transform=transform)
    train_data_len = int(len(all_data) * 0.75)
    valid_data_len = int((len(all_data) - train_data_len) / 2)
    test_data_len = int(len(all_data) - train_data_len - valid_data_len)
    train_data, val_data, test_data = random_split(
        all_data, [train_data_len, valid_data_len, test_data_len]
    )
    train_loader = DataLoader(train_data, batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(val_data, batch_size=batch_size, shuffle=True)
    test_loader = DataLoader(test_data, batch_size=batch_size, shuffle=True)
    return ((train_loader, val_loader, test_loader), all_data.classes)


(train_loader, val_loader, test_loader), classes = get_data_loaders(DATASET_DIR, 64)
print("Dataset classes:")
print(classes)

device = "cuda:0" if torch.cuda.is_available() else "cpu"
print(f"Device is {device}")

# For MobileNet v2.
model = models.mobilenet_v2(pretrained=True)
# Remap output layer to number of classes in dataset.
n_inputs = model.classifier[1].in_features
last_layer = nn.Linear(n_inputs, len(classes))
model.classifier = last_layer

# For DarkNet19.
#model = darknet.Darknet19(pretrained=False, n_classes=len(classes))

print(model)

writer = SummaryWriter(log_dir=LOG_DIR)

if torch.cuda.is_available():
    model.cuda()
print(f"Remapped output features: {model.classifier}")
print(f"Remapped model: {model}")

wandb.watch(model)

def apply_test_transforms(inp):
    out = transforms.functional.resize(inp, [224, 224])
    out = transforms.functional.to_tensor(out)
    out = transforms.functional.normalize(
        out, [0.485, 0.456, 0.406], [0.229, 0.224, 0.225]
    )
    return out


def predict(model, filepath, show_img=False, url=False):
    if url:
        response = requests.get(filepath)
        im = Image.open(BytesIO(response.content))
    else:
        im = Image.open(filepath)
    if show_img:
        plt.imshow(im)
    im_as_tensor = apply_test_transforms(im)
    minibatch = torch.stack([im_as_tensor])
    if torch.cuda.is_available():
        minibatch = minibatch.cuda()
    pred = model(minibatch)
    print(f"MDW: prediction is: {pred}")
    _, classnum = torch.max(pred, 1)
    topclass = classes[classnum]
    print(f"MDW: Top class is: {classnum} {topclass}")
    return classes[classnum]


# This is a quick pre-training check.
predict(
    model,
    DATASET_DIR + "3005-Dark_Pink/image00142.png",
    show_img=False,
)

criterion = nn.CrossEntropyLoss()
#optimizer = optim.SGD(model.classifier.parameters(), lr=INITIAL_LR, momentum=MOMENTUM)
optimizer = optim.RMSprop(model.classifier.parameters(), lr=INITIAL_LR, momentum=MOMENTUM)
scheduler = StepLR(optimizer, step_size=((MAX_EPOCHS * 0.9) // 3), gamma=GAMMA)
training_history = {"accuracy": [], "loss": []}
validation_history = {"accuracy": [], "loss": []}
trainer = create_supervised_trainer(model, optimizer, criterion, device=device)
evaluator = create_supervised_evaluator(
    model,
    device=device,
    metrics={
        "accuracy": Accuracy(),
        "loss": Loss(criterion),
        "cm": ConfusionMatrix(len(classes)),
    },
)

print(f"Trainer is: {trainer}")
print(f"Evaluator is: {evaluator}")


@trainer.on(Events.EPOCH_COMPLETED)
def step_lr_scheduler(engine):
    scheduler.step()
    ldict = {}
    # Log current learning rate.
    for index, param_group in enumerate(optimizer.param_groups):
        ldict[f"lr_{index}"] = param_group["lr"]
    wandb.log(ldict)


@trainer.on(Events.ITERATION_COMPLETED)
def log_training_loss(engine):
    print(
        "Epoch[{}] Iteration[{}/{}] Loss: {:.2f}"
        "".format(
            engine.state.epoch,
            engine.state.iteration,
            len(train_loader),
            engine.state.output,
        )
    )
    writer.add_scalar("training/loss", engine.state.output, engine.state.iteration)
    wandb.log({"train loss": engine.state.output})


@trainer.on(Events.EPOCH_COMPLETED)
def log_training_results(trainer):
    evaluator.run(train_loader)
    metrics = evaluator.state.metrics
    accuracy = metrics["accuracy"] * 100
    loss = metrics["loss"]
    training_history["accuracy"].append(accuracy)
    training_history["loss"].append(loss)
    print()
    print(
        "Training Results - Epoch: {}  Avg accuracy: {:.2f} Avg loss: {:.2f}".format(
            trainer.state.epoch, accuracy, loss
        )
    )
    writer.add_scalar("training/avg_loss", loss, trainer.state.epoch)
    writer.add_scalar("training/avg_accuracy", accuracy, trainer.state.epoch)
    wandb.log({"train avg_loss": loss})
    wandb.log({"train avg_accuracy": accuracy})


@trainer.on(Events.EPOCH_COMPLETED)
def log_validation_results(trainer):
    evaluator.run(val_loader)
    metrics = evaluator.state.metrics
    accuracy = metrics["accuracy"] * 100
    loss = metrics["loss"]
    validation_history["accuracy"].append(accuracy)
    validation_history["loss"].append(loss)
    print(
        "Validation Results - Epoch: {}  Avg accuracy: {:.2f} Avg loss: {:.2f}".format(
            trainer.state.epoch, accuracy, loss
        )
    )
    writer.add_scalar("valdation/avg_loss", loss, trainer.state.epoch)
    writer.add_scalar("valdation/avg_accuracy", accuracy, trainer.state.epoch)
    wandb.log({"validation avg_loss": loss})
    wandb.log({"validation avg_accuracy": accuracy})


# Add checkpointing.
checkpointer = ModelCheckpoint(
    CHECKPOINT_DIR,
    WORKLOAD,
    n_saved=None,
    create_dir=True,
    save_as_state_dict=True,
    require_empty=False,
)
trainer.add_event_handler(Events.EPOCH_COMPLETED, checkpointer, {WORKLOAD: model})


# Do a quick training test run.
print("Starting training run...")
trainer.run(train_loader, max_epochs=MAX_EPOCHS)
print("Done.")

writer.close()
