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
const numStrips = 100;
const ledsPerStrip = 100;

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
    game.time.desiredFps = 200;
    //game.time.slowMotion = slowMotion;

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
  if (x < 0 || x >= ledsPerStrip) {
    return;
  }
  if (y < 0 || y >= numStrips) {
    return;
  }
  //console.log("setLed: " + x + "," + y + " -> ", c);
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

function fadeTo(x, y, color) {
  var led = getLed(x, y);
  var cur = led.tint;
  var diff = color - cur;
  var next = Math.floor(cur + (diff / 4));
  setLed(x, y, next);
}

function spackle() {
  var x = getRandomInt(0, ledsPerStrip);
  var y = getRandomInt(0, numStrips);
  setLed(x, y, 0xff0000);
}

var sweepColors = [];
function updateLeds() {
  if (sweepColors.length == 0) {
    for (y = 0; y < numStrips; y++) {
      sweepColors[y] = Math.random() * 0xffffff;
    }
  }
  for (y = 0; y < numStrips; y++) {
    var s = getRandomInt(0, ledsPerStrip);
    var dir = (Math.random() < 0.5) ? -1 : 1;
    sweep(s, dir, y, sweepColors[y], 4);
  }
}

function interpolate(c1, c2, weight) {
  c1r = (c1 & 0xff0000) >> 16;
  c1g = (c1 & 0x00ff00) >> 8;
  c1b = (c1 & 0x0000ff);

  c2r = (c2 & 0xff0000) >> 16;
  c2g = (c2 & 0x00ff00) >> 8;
  c2b = (c2 & 0x0000ff);

  tr = Math.floor(c1r + (c2r - c1r) * weight);
  tg = Math.floor(c1g + (c2g - c1g) * weight);
  tb = Math.floor(c1b + (c2b - c1b) * weight);
  return (tr << 16) | (tg << 8) | tb;
}

var sweepTick = [];
var sweepDir = [];
function sweep(start, dir, y, color, steps) {
  if (sweepTick[y] == null) {
    sweepTick[y] = start;
  }
  if (sweepDir[y] == null) {
    sweepDir[y] = dir;
  }

  setLed(sweepTick[y], y, color);
  setLed(sweepTick[y] + 1*(-sweepDir[y]), y, interpolate(color, 0, 0.1));
  setLed(sweepTick[y] + 2*(-sweepDir[y]), y, interpolate(color, 0, 0.3));
  setLed(sweepTick[y] + 3*(-sweepDir[y]), y, interpolate(color, 0, 0.5));
  setLed(sweepTick[y] + 4*(-sweepDir[y]), y, interpolate(color, 0, 0.7));
  setLed(sweepTick[y] + 5*(-sweepDir[y]), y, interpolate(color, 0, 0.9));
  setLed(sweepTick[y] + 6*(-sweepDir[y]), y, 0);

  sweepTick[y] += sweepDir[y];
  if (sweepTick[y] < 0) {
    sweepTick[y] = 0;
    sweepDir[y] = 1;
  } else if (sweepTick[y] >= ledsPerStrip) {
    sweepTick[y] = ledsPerStrip - 1;
    sweepDir[y] = -1;
  }
}

