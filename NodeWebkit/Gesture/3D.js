
function prepareSocketConnection() {
	
	window.s = require('net').Socket();

	console.log(window.s);
	
	'169.254.154.163 billy' 
	'169.254.231.36 me'


	try {
        window.s.connect(3490, '127.0.0.1', function(d){
		    console.log("connected");
		    console.log(window.s.address(),window.s.remoteAddress)
		});
    } catch(e) {
        console.log(e);
    }
	

	window.s.on('data', function(d){
		if (d.toString() == "destroy" || d.toString() == "destroydestroy") {
			scene.remove(window.box);
			window.box.remove();
			window.box = null;
			window.addBox();
			return;
		}

		var data = d.toString().split(" ").map(function(e){
			return parseFloat(e);
		});
		

		var matrix = getMatrix(data)
		window.box.applyMatrix(matrix);

	});

	//window.s.end();
}


function getMatrix(data) {
	var transform1 = new THREE.Matrix4(),
	transform2 = new THREE.Matrix4(),
	transform3 = new THREE.Matrix4(),
	transform = new THREE.Matrix4(),
	transformBack = new THREE.Matrix4();

	
	data[1] = -1 * data[1];
	data[2] = -1 * data[2];
	window.box.scale.y = window.box.scale.x;
	window.box.scale.z = window.box.scale.x;

	if ((window.box.scale.x < 0.5 && data[0] > 0.5) || (window.box.scale.x > 2.7 && data[0] < 2.7) || (window.box.scale.x <= 2.7 && window.box.scale.x >= 0.5)) {
		transformBack.scale(new THREE.Vector3(1/window.box.scale.x,1/window.box.scale.y,1/window.box.scale.z));
		transform1.scale(new THREE.Vector3( data[0],data[0],data[0]));
	} 

	
	transform2.makeRotationZ(data[1]);
	transform3.makeRotationY(data[2]);

	transform.multiply(transformBack);
	transform.multiply(transform1);
	transform.multiply(transform2);
	transform.multiply(transform3);

	return transform;
}


