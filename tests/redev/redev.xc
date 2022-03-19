interface participant:
    integer<1> data
    
    gen_data():
        out: data

interface app:
    integer<1> data

domain Global:
    range: [0, 16777215]

component Participant(participant)[participant]
component App(app)[app]

Participant.data := Global
App.data := Global

Participant.data.%{t}:
    Participant.gen_data() : out=data.%{t}

App.data.%{t}: Participant.data.%{t}
    App.data.%{t} < Participant.data.%{t}

Participant@step.%{t}:
    Participant.data.%{t}

App@step.%{t}:
    App.data.%{t}
