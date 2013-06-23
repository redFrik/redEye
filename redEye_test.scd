n= NetAddr("127.0.0.1", 43334)
n.sendMsg(\rotate, 1, 2, 4.0.rand2)
n.sendMsg(\light, 1)
n.sendMsg(\colorFront, 1.0.rand, 1.0.rand, 1.0.rand, 0.9)
n.sendMsg(\width, 10)
n.sendMsg(\lightDir, 1.0.rand, 1.0.rand, 1.0.rand)

(
r= Routine.run({
	999.do{
		n.sendMsg(\colorFront, 0.1, 1.0.rand, 1.0.rand, 0.9);
		(1/60).wait;
	};
});
)
r.stop

(
r= Routine.run({
	9999.do{|i|
		n.sendMsg(\rotate, sin(i*0.1), sin(i*0.11), sin(i*0.12));
		(1/60).wait;
	};
});
)
r.stop

n.sendMsg(\mTranslate, 1.1, 1.2, 1.3)