#!/usr/bin/env python

import os

import numpy as np
import pandas as pd

import torch
import torchvision
from torchvision import datasets, transforms
from torch import nn, optim
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
from ignite.handlers import Checkpoint, ModelCheckpoint, EarlyStopping
from ignite.metrics import Accuracy, Loss, RunningAverage, ConfusionMatrix
from ignite.contrib.handlers import ProgressBar

LOG_DIR = "/home/mdw/tensorboard/"
DATASET_DIR = "/home/mdw/datasets/CUB_200_2011/images/"
CHECKPOINT_DIR = "/home/mdw/checkpoints/"
MAX_EPOCHS = 1000
BATCH_SIZE = 64


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


(train_loader, val_loader, test_loader), classes = get_data_loaders(
    DATASET_DIR, BATCH_SIZE
)
print("Dataset classes:")
print(classes)

model = models.mobilenet_v2(pretrained=True)
device = "cuda:0" if torch.cuda.is_available() else "cpu"
print(f"Device is {device}")

model = models.mobilenet_v2(pretrained=True)
print(model)
print(f"Input features: {model.classifier[1].in_features}")
print(f"Output features: {model.classifier[1].out_features}")

# Remap output layer to number of classes in dataset.
n_inputs = model.classifier[1].in_features
last_layer = nn.Linear(n_inputs, len(classes))
model.classifier = last_layer
if torch.cuda.is_available():
    model.cuda()
print(f"Remapped output features: {model.classifier}")

all_checkpoints = list(
    sorted(
        [
            f
            for f in os.listdir(CHECKPOINT_DIR)
            if os.path.isfile(os.path.join(CHECKPOINT_DIR, f))
        ]
    )
)
last_checkpoint = "beaker_beaker_10981.pt"
print(f"Loading checkpoint: {last_checkpoint}")
checkpoint = torch.load(os.path.join(CHECKPOINT_DIR, last_checkpoint))
Checkpoint.load_objects(to_load={"beaker": model}, checkpoint=checkpoint)


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
    DATASET_DIR + "080.Green_Kingfisher/Green_Kingfisher_0027_71048.jpg",
    show_img=False,
)

# Run a Validation pass.
criterion = nn.CrossEntropyLoss()
optimizer = optim.SGD(model.classifier.parameters(), lr=0.001, momentum=0.9)
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
evaluator.run(val_loader)
metrics = evaluator.state.metrics
accuracy = metrics["accuracy"] * 100
loss = metrics["loss"]
validation_history["accuracy"].append(accuracy)
validation_history["loss"].append(loss)
print(
    "Validation Results - Avg accuracy: {:.2f} Avg loss: {:.2f}".format(accuracy, loss)
)

scripted_model = torch.jit.script(model)
print(scripted_model.code)
scripted_model.save("beaker_torchscript.zip")
print("Saved TorchScript model to beaker_torchscript.zip")

x = torch.randn(BATCH_SIZE, 3, 224, 224, requires_grad=True, device=device)

#import time
#t1 = time.time()
#for _ in range(100):
#  _ = model(x)
#t2 = time.time()
#ms_per_iter = ((t2-t1)*1000) / 100.0
#print(f"Ran 100 iterations in {t2-t1} sec, {ms_per_iter} ms/iter")

torch.onnx.export(
    model,
    x,
    "beaker.onnx",
    export_params=True,
    opset_version=10,
    do_constant_folding=True,
    input_names=["input"],
    output_names=["output"],
    dynamic_axes={
        "input": {0: "batch_size"},
        "output": {0: "batch_size"},
    },
)

print("Saved ONNX model to beaker.onnx")
