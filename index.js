const nosolocyber = require('./build/Release/nosolocyber.node');

nosolocyber.initEventHook(function(obj){
	console.log(obj);
});

module.exports = nosolocyber;