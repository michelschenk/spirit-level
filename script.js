var myGamePiece;

function startGame() {
    myGamePiece = new component(30, 30, "red", 125, 125);
    myGameArea.start();
}

var myGameArea = {
    canvas : document.createElement("canvas"),
    start : function() {
        this.canvas.width = 270;
        this.canvas.height = 270;
        this.context = this.canvas.getContext("2d");
        document.body.insertBefore(this.canvas, document.body.childNodes[0]);
        this.frameNo = 0;
		updateGameArea();
		if (!!window.EventSource) {
			var source = new EventSource('/events');
			source.addEventListener('open', function(e) {
				console.log("Events Connected");
			}, false);
			source.addEventListener('error', function(e) {
				if (e.target.readyState != EventSource.OPEN) {
					console.log("Events Disconnected");
				}
			}, false);
			source.addEventListener('message', function(e) {
				console.log("message", e.data);
			}, false);
			//const e = '{"x":25.3, "y":2.0}'
			source.addEventListener('xy', function(e) {
				console.log("xy : ", e.data);
				obj = JSON.parse(e.data);
				myGameArea.x = 135 + obj.x*10;
				myGameArea.y = 135 + obj.y*10;
				myGameArea.z = obj.z;
				myGameArea.t = obj.t;
				updateGameArea();
			}, false);
		}
    },
    stop : function() {
        clearInterval(this.interval);
    },    
    clear : function() {
    	var ctx = this.context;
        var hw = this.canvas.width;
        var hh = this.canvas.height;
        ctx.beginPath();
        ctx.clearRect(0, 0, hw, hh);
        ctx.stroke();
    }
}

function component(width, height, color, x, y) {
    this.width = width;
    this.height = height;
    this.speedX = 0;
    this.speedY = 0;    
    this.x = x;
    this.y = y;  
	//this.z = z;	
    this.update = function() {
        ctx = myGameArea.context;
        hw = myGameArea.canvas.width;
        hw2 = hw/2;
        hh = myGameArea.canvas.height;
        hh2 = hh/2;
        this.x = (this.x>hw) ? hw : this.x;
        this.y = (this.y>hh) ? hh : this.y;
        this.x = (this.x<0) ? 0 : this.x;
        this.y = (this.y<0) ? 0 : this.y;
        ctx.save();
        ctx.beginPath();
        ctx.globalAlpha = 1;
        var grd = ctx.createRadialGradient(this.x, this.y, 5, this.x, this.y, 30);
		grd.addColorStop(0, "#6fcc42");
		grd.addColorStop(1, "#daf11c");
        ctx.fillStyle = grd;
        //ctx.fillStyle = "green";
        ctx.shadowBlur = 20;
        ctx.shadowColor = "black";
        ctx.arc(this.x, this.y, 23, 0, 2 * Math.PI);
        ctx.fill();
        ctx.globalAlpha = 1.0;
        ctx.shadowBlur = 10;
        ctx.shadowColor = "black";
		ctx.beginPath();
		ctx.arc(hw2 , hh2, 98, (this.z-2)*(Math.PI/180), (this.z+2)*(Math.PI/180));
		ctx.stroke();
        const dd=[hw2,0,hw2,hh2-30,hw2,hh2+30,hw2,hh,hw,hh2,hw2+30,hh2,0,hh2,hw2-30,hh2,
                  hw, hh2-25, hw-15, hh2-25, hw, hh2+25, hw-15, hh2+25, hw, hh2-35, hw-15, hh2-35, hw, hh2+35, hw-15, hh2+35,
                  hw2-25, hh, hw2-25, hh-15, hw2+25, hh, hw2+25, hh-15, hw2-35, hh, hw2-35, hh-15, hw2+35, hh, hw2+35, hh-15];

		ctx.beginPath();
        
        for (let i = 0; i < dd.length; i=i+4) {
        	ctx.moveTo(dd[i], dd[i+1]);
        	ctx.lineTo(dd[i+2], dd[i+3]);
		}
        
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(hw/2, hh/2, 25, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(hw/2, hh/2, 60, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(hw/2, hh/2, 95, 0, 2 * Math.PI);
        ctx.stroke();
        ctx.shadowBlur = 5;
        ctx.restore();
        document.getElementsByTagName("p")[0].innerHTML = this.t;
    }
}

function updateGameArea() {
    myGameArea.clear();
    if (myGameArea.x && myGameArea.y) {
        myGamePiece.x = myGameArea.x;
        myGamePiece.y = myGameArea.y;        
        myGamePiece.z = myGameArea.z;        
        myGamePiece.t = myGameArea.t;        
    }
    myGamePiece.update();
}
