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
Left.u.%{t}: Left.u.%[[t-1]] Left.du.%[[t-1]]
    Left.advance(dt)
Left.du.%{t}: Left.u.%{t} Right.u.%{t}
    Left.u.%{t} < Right.u.%{t}
    Left.solve()

Right.u.0:
Right.du.%{t}: Left.u.%[[t+1]] Right.u.%{t}
    Right.u.%{t} < Left.u.%[[t+1]]
    Right.solve()
Right.u.%{t}: Right.u.%[[t-1]] Right.du.%[[t-1]]
    Right.advance(dt)

Left@ts.%{t}:
    Left.u.%{t}

Right@ts.%{t}:
    Right.u.%{t}
