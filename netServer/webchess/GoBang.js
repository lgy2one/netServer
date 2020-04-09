require=function(r,e,n){
	function t(n,o){
		function i(r){
			return t(i.resolve(r))
		}
		function f(e){
			return r[n][1][e]||e
		}
		if(!e[n]){
			if(!r[n]){
				var c="function"==typeof require&&require;
				if(!o&&c)return c(n,!0);
				if(u)return u(n,!0);
				var l=new Error("Cannot find module '"+n+"'");
				throw l.code="MODULE_NOT_FOUND",l}i.resolve=f;
					var s=e[n]=new t.Module(n);
					r[n][0].call(s.exports,i,s,s.exports)
				}
				return e[n].exports
			}
			function o(r){
				this.id=r,this.bundle=t,this.exports={}
			}
			var u="function"==typeof require&&require;
			t.isParcelRequire=!0,t.Module=o,t.modules=r,t.cache=e,t.parent=u;
			for(var i=0;i<n.length;i++)
				t(n[i]);
			return t
		}({5:[function(require,module,exports) {
			"use strict";
			Object.defineProperty(exports,"__esModule",{value:!0}),exports.GameState=void 0;
			var e=function(){
				function e(e,t){
					for(var r=0;r<t.length;r++){
						var a=t[r];a.enumerable=a.enumerable||!1,a.configurable=!0,"value"in a&&(a.writable=!0),Object.defineProperty(e,a.key,a)
					}
				}
				return function(t,r,a){
					return r&&e(t.prototype,r),a&&e(t,a),t
				}
			}(),t=require("./main");
			function r(e,t){
				if(!(e instanceof t))throw new TypeError("Cannot call a class as a function")
			}
			var a=0,n=exports.GameState=function(){
				function a(e,t,n,i,h){
					r(this,a),this.board=e,this.playerTurn=t,this.depth=n,this.choosenState=null,this.winner=!1,this.alpha=i||-1/0,this.beta=h||1/0,this.counter=0
				}return e(a,[{key:"getScore",value:function(){
					var e=(0,t.getAiToken)(),r=this.checkFinish();
					if(r)return"draw"===r?0:r===e?10:-10;
					if(this.depth>=100)return 0;
					var n=_.shuffle(this.getAvailablePos());
					if(this.playerTurn===e){
						for(var h=-1e3,o=0;o<n.length;o++){
							var s=n[o],u=new a(this.generateNewBoard(s,this.playerTurn),i(this.playerTurn),this.depth+1,this.alpha,this.beta),l=u.getScore();
							if(l>h&&(h=l,o,this.choosenState=u,this.alpha=h),this.alpha>=this.beta)break
						}
						return h
					}
					for(var c=1e3,f=0;f<n.length;f++){
						var p=n[f],v=new a(this.generateNewBoard(p,this.playerTurn),i(this.playerTurn),this.depth+1,this.alpha,this.beta),b=v.getScore();
						if(b<c&&(c=b,f,this.choosenState=v,this.beta=c),this.alpha>=this.beta)break
					}
					return c
				}
			},{key:"checkFinish",value:function(){for(var e=this.board,t=0;t<9;t+=3)if(e[t]&&e[t]===e[t+1]&&e[t]===e[t+2])return e[t];for(var r=0;r<3;r++)if(e[r]&&e[r]===e[r+3]&&e[r]===e[r+6])return e[r];return e[0]&&e[0]===e[4]&&e[0]===e[8]?e[0]:e[2]&&e[2]===e[4]&&e[2]===e[6]?e[2]:9===_.compact(e).length&&"draw"}},{key:"getAvailablePos",value:function(){var e=[];return this.board.forEach(function(t,r){t||e.push(r)}),e}},{key:"generateNewBoard",value:function(e,t){var r=_.clone(this.board);return r[e]=t,r}},{key:"nextMove",value:function(){this.board=_.clone(this.choosenState.board)}}]),a}();function i(e){return e===t.X?t.O:t.X}
			},{"./main":3}],6:[function(require,module,exports) {
			"use strict";Object.defineProperty(exports,"__esModule",{value:!0}),
				exports.px2Index=t,
				exports.drawGrid=r,
				exports.initCanvas=l,
				exports.draw=a;
			var e=0,o=0;
			function t(t,r){
				var n=arguments.length>2&&void 0!==arguments[2]?arguments[2]:GRID_SIZE,
					i=e,
					l=o;
				return{
					col:Math.floor((t-i)/n),row:Math.floor((r-l)/n)}}function r(e,o,t){
						var r=arguments.length>3&&void 0!==arguments[3]?arguments[3]:GRID_SIZE,
							n=arguments.length>4&&void 0!==arguments[4]?arguments[4]:"#707070",
							i=arguments.length>5&&void 0!==arguments[5]?arguments[5]:0,
							l=arguments.length>6&&void 0!==arguments[6]?arguments[6]:0;
						e.strokeStyle=n,
						e.lineWidth=3,
						e.beginPath(),
						e.moveTo(i-.5,l),
						e.lineTo(i-.5,t*r+l);
						for(var a=0;a<o+1;a++)e.moveTo(a*r-.5+i,l),e.lineTo(a*r-.5+i,t*r+l);e.stroke(),e.moveTo(i,l-.5),e.lineTo(o*r+i,l-.5);for(var d=0;d<t+1;d++)e.moveTo(i,d*r-.5+l),e.lineTo(o*r+i,d*r-.5+l);e.stroke()}var n=exports.canvas=document.getElementById("board"),i=exports.ctx=n.getContext("2d");function l(){n.width=450,n.height=450}function a(e){i.clearRect(0,0,n.width,n.height),r(i,3,3,150),i.font="40px Arial",e.forEach(function(e,o){e&&(i.fillStyle="X"===e?"red":"green",i.fillText(e,150*(o%3+.4),150*(Math.floor(o/3)+.6)))})}
			},{}],3:[function(require,module,exports) {
			"use strict";Object.defineProperty(exports,"__esModule",{value:!0}),exports.O=exports.X=void 0,exports.getAiToken=v;var e=require("./alphabeta"),n=require("./draw"),t=exports.X="X",r=exports.O="O",o=r,a=new e.GameState(i,o),i=_.fill(Array(9),null);function c(e){var t=n.canvas.getBoundingClientRect(),r=e.clientX-t.left,o=e.clientY-t.top,a=(0,n.px2Index)(r,o,150),i=a.col;u(a.row,i)}function d(e,n){return 3*e+n}function u(e,i){var c=d(e,i);if(!a.board[c]&&!a.winner){var u=_.clone(a.board);u[c]=o===t?r:t,a.board=_.clone(u),(0,n.draw)(a.board),s(),setTimeout(l,1e3)}}function l(){a.winner||(a.getScore(),a.nextMove(),(0,n.draw)(a.board),s())}function s(){var e=a.checkFinish();e&&(console.log("游戏结束",e),document.getElementById("winner").innerHTML="draw"===e?"平局":"获胜的棋子是: "+e,a.winner=e)}function m(n){var c=n.target.dataset.token;o=c===t?r:t,c===r&&(a=new e.GameState(i,o),w(c),setTimeout(l,0))}function w(e){document.getElementById("show-choice").innerHTML="你的棋子选择: "+e}function v(){return o}function f(){w((o=r)===t?r:t),a=new e.GameState(i,o),(0,n.draw)(a.board),document.getElementById("winner").innerHTML=""}function g(){(0,n.initCanvas)(),a=new e.GameState(i,o,0),(0,n.draw)(a.board);var t=document.getElementById("btn-choose-o"),r=document.getElementById("btn-choose-x");t.addEventListener("click",m),r.addEventListener("click",m),document.getElementById("restart-game").addEventListener("click",f)}n.canvas.addEventListener("click",c,!1),g();
			},{"./alphabeta":5,"./draw":6}]
		},{},[3])
