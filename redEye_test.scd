n= NetAddr("127.0.0.1", 43334)
n.sendMsg(\rotate, 1, 2, 4.0.rand2)
n.sendMsg(\color0, 1.0.rand, 1.0.rand, 1.0.rand, 0.9)
n.sendMsg(\colorBack, 1.0.rand, 1.0.rand, 1.0.rand, 0.9)
n.sendMsg(\width, 10)

(
r= Routine.run({
	999.do{
		n.sendMsg(\color0, 0.1, 1.0.rand, 1.0.rand, 0.9);
		(1/60).wait;
	};
});
)
r.stop

(
r= Routine.run({
	9999.do{|i|
		n.sendMsg(\rotate0, sin(i*0.1), sin(i*0.11), sin(i*0.12));
		(1/60).wait;
	};
});
)
r.stop

n.sendMsg(\translate0, 300.1, 10.2, 1.3)