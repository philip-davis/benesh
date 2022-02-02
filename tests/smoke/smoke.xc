interface simtest:
    real<1> u
    real incr

    calc_incr():
        in: u
        out: incr

    advance(real):
        in: u, incr
        out: u

interface antest:
    real<1> u
    real stat

    analyze():
        in: u
        out: stat

domain Global:
    pitch: 0.00469970703125
    range: [0., 0.996337890625]
    domain LeftSide:
        decomposition: regular:{1}
        range: [0., 0.6015625]
        ghost: 0.00469970703125
    domain RightSide:
        decomposition: regular:{1}
        range: [0.394775390625,1.]
        ghost: 0.00469970703125

component Sim(simtest)[tptest]
component Analysis(antest)[tptest2]

Sim.u := Global.LeftSide
Analysis.u := Global.RightSide

real dt = -0.00048828125

Analysis.u.1:
    dt = dt + 1
    Analysis.analyze()
Sim.u.%{t}:
    Sim.calc_incr()
    Sim.advance(2)
Analysis.u.2: Sim.u.1
    dt = 2
    Analysis.u.2 < Sim.u.1
    Analysis.analyze() : in = u.2 ; out = stat.2

Sim@stage.%{t}.%{s}:
    Sim.u.1

Analysis@test.%{t}:
    Analysis.u.%{t}
