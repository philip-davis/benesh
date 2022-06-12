interface client:
    real<2> data
    
    gen_data():
        out: data

interface cpl:
    real<2> data

domain Global:
    mesh: d3d-full_9k_sfc
    domain core:
        class: <22
    domain edge:
        class: >34
    domain overlap:
        class: 22-34

component Client1(client)[client1]
component Client2(client)[client2]
component Coupler(cpl)[coupler]

Client1.data := Global
Client2.data := Global
Coupler.data := Global

Client1.data.%{t}:
    Client1.gen_data() : out=data.%{t}

Client2.data.%{t}:
    Client2.gen_data() : out=data.%{t}

App.data.%{t}: Participant.data.%{t}
    Coupler.data.%{t} < Client1.data.%{t}
    Coupler.data.%{t} < Client2.data.%{t}

Client1@step.%{t}:
    Client1.data.%{t}

Client2@step.%{t}:
    Client2.data.%{t}

Coupler@step.%{t}:
    Coupler.data.%{t}
    
