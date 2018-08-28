var SimState = function () {};

SimState.prototype = {
  preload: preload,
  create: create,
  update: update,
  render: render,
};

const worldWidth = window.innerWidth * window.devicePixelRatio;
const worldHeight = window.innerHeight * window.devicePixelRatio;

const ledSize = 10;
const ledGap = 15;
const stripGap = 20;
const numStrips = 1;
const ledsPerStrip = 60;

const slowMotion = 1.0;

var tick = 0;
var ledTexture;
var stripGroup;
var led;

var debugString = "";

function preload() {
}

function create() {
    // We're going to be using physics, so enable the Arcade Physics system
    game.physics.startSystem(Phaser.Physics.ARCADE);

    game.time.advancedTiming = true;
    game.time.desiredFps = 60;
    game.time.slowMotion = slowMotion;

    game.world.setBounds(0, 0, worldWidth, worldHeight);

    //background.tint = 0x202020;

    ledTexture = new Phaser.Graphics(game, 0, 0)
      .beginFill(0xffffff, 1.0)
      .drawCircle(0, 0, ledSize)
      .endFill()
      .generateTexture();

    stripGroup = game.add.group();
    stripGroup.enableBody = true;
    makeStrips();

    pauseKey = game.input.keyboard.addKey(Phaser.Keyboard.SPACEBAR);
    pauseKey.onDown.add(pauseGame, this);

    // Stack things.
    game.world.bringToTop(strips);
}

function pauseGame() {
  if (!game.paused) {
    game.paused = true;
  } else {
    game.paused = false;
  }
}

function makeStrips() {
  strips = [];
  for (i = 0; i < numStrips; i++) {
    var strip = [];
    strips.push(strip);
    for (j = 0; j < ledsPerStrip; j++) {
      var led = stripGroup.create(ledGap+(j * ledGap), stripGap+(i * stripGap),
        ledTexture);
      led.anchor.setTo(.5,.5);
      strip.push(led);
    }
  }
  clearLeds();
}

function update() {
  updateLeds();
}

function render() {
  if (debugString != '') {
    game.debug.text(debugString, 20, 20);
  }
}

function setLed(x, y, c) {
  console.log("setLed: " + x + "," + y + " -> ", c);
  strips[y][x].tint = c;
}

function getLed(x, y) {
  return strips[y][x];
}

function setAll(c) {
  for (i = 0; i < strips.length; i++) {
    for (j = 0; j < strips[i].length; j++) {
      setLed(j, i, c);
    }
  }
}

function clearLeds() {
  setAll(0);
}

function getRandomInt(min, max) {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min)) + min;
}

function spackle() {
  var x = getRandomInt(0, ledsPerStrip);
  var y = getRandomInt(0, numStrips);
  setLed(x, y, 0xff0000);
}

function updateLeds() {
  spackle();
}

