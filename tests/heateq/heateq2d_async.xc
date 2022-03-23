interface heateq:
    real<2> u
    real<2> du

    solve():
        in: u
        out: du

    advance(real):
        in: u, du
        out: u

domain Global:
    range: [0., 1.09375] * [0., 1.09375]
    domain LeftSide:
        range: [0., 1.09375] * [0., 0.625]
    domain RightSide:
        range: [0., 1.09375] * [0.46875, 1.09375]

component Left(heateq)[heat1]
component Right(heateq)[heat2]

Left.u := Global.LeftSide
Left.du := Global.LeftSide
Right.u := Global.RightSide
Right.du := Global.RightSide

real dt = 0.00000000001

Left.u.0:
Left.du.0: Left.u.0
    Left.solve()
Left.u.1: Left.u.0 Left.du.0
    Left.advance(dt) : in=u.0, du.0 ; out=u.1
Left.du.1: Left.u.1 Right.u.0
    Left.u.1 < Right.u.0
    Left.solve()
Left.u.2: Left.u.1 Left.du.1
    Left.advance(dt)
Left.du.2: Left.u.2 Right.u.1
    Left.u.2 < Right.u.2
    Left.solve()
Left.u.3: Left.u.2 Left.du.2
    Left.advance(dt)
Left.du.3: Left.u.3 Right.u.2
    Left.u.3 < Right.u.2
    Left.solve()
Left.u.4: Left.u.3 Left.du.3
    Left.advance(dt)
Left.du.4: Left.u.4 Right.u.3
    Left.u.4 < Right.u.4
    Left.solve()

Right.u.0:
Right.du.1: Left.u.0 Right.u.0
    Right.u.0 < Left.u.1
    Right.solve()
Right.u.1: Right.u.0 Right.du.1
    Right.advance(dt)
Right.du.2: Left.u.2 Right.u.1
    Right.u.1 < Left.u.2
    Right.solve()
Right.u.2: Right.u.1 Right.du.2
    Right.advance(dt)
Right.du.3: Left.u.3 Right.u.2
    Right.u.2 < Left.u.3
    Right.solve()
Right.u.3: Right.u.2 Right.du.3
    Right.advance(dt)
Right.du.4: Left.u.4 Right.u.3
    Right.u.3 < Left.u.4
    Right.solve()
Right.u.4: Right.u.3 Right.du.4
    Right.advance()

Left@ts.%{t}:
    Left.u.%{t}

Right@ts.%{t}:
    Right.u.%{t}
