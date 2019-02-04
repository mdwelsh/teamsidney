var game = new Phaser.Game(2200, 1000, Phaser.AUTO, 'game-container');

game.state.add('sim', SimState);

WebFont.load({
  google: {
    families: ['Bubbler One', 'Russo One']
  },
  active: function() { 
    console.log('Starting sim state');
    game.state.start('sim');
  },
});

var fullScreen = false;
function goFullScreen() {
  console.log('goFullScreen called, desktop is ' + game.device.desktop +
      ', fullScreen is ' + fullScreen);
  if (game.device.desktop) {
    return;
  }
  if (fullScreen) {
    return;
  }
  game.scale.fullScreenScaleMode = Phaser.ScaleManager.SHOW_ALL;
  game.scale.onFullScreenInit.add(function(e, target) {
    console.log('onFullScreenInit');
    console.log(e);
    console.log(target);
  });
  game.scale.onFullScreenError.add(function(e) {
    console.log('onFullScreenError');
    console.log(e);
  });
  game.scale.onFullScreenChange.add(function(e) {
    console.log('onFullScreenChange');
    console.log(e);
  });
  console.log('Trying to go full screen');
  ret = game.scale.startFullScreen();
  console.log('startFullScreen returned ' + ret);
  fullScreen = true;
}
