module Ventilador

export TunelRPM, connect, run!,stop!,sendRPM!
using MQTTClient


rpm_topic = "tunel/rpm"         # Topico RPM
status_topic = "tunel/status"   # Topico Status

 ## --------- Função para conectar no broker --------------------------#
    

 





function callback(topic,payload)
    return println("Mensagem recebida no topico: [",topic,"] mensagem: [",String(payload),"]")

end

mutable struct TunelRPM   ## Estrutura de dados para armazenar as informações do ventilador
    "Nome do dispositivo"
    name::String
    "Broker MQTT usado"
    broker::String          #IP do broker
    "Porta do broker MQTT"
    port::Int               # Porta do broker
    "Usuário do MQTT. Vazio se for anônimo"
    user::String
    "Senha do MQTT. Vazio se for anônimo"
    passwd::String
    "Tópico contendo RPM"
    topic_rpm::String       #Topico do RPM
    topic_status::String    # Tópico do status
    "Cliente do MQTT"
    client::Client
    """
    Status da última programação:
    (RPM, Status)
    RPM - Inteiro com o RPM programado
    Status - Bool, true se estiver run, false se estiver stop
    """
    status::Tuple{Int,Bool}
end


function TunelRPM(toml::AbstractDict)
    broker = toml["broker"]
    ks = keys(toml)

    if "name" ∉ ks
        error("O campo `name` deve ser configurado!")
    else
        name = toml["name"]
    end
    
    if "port" in ks
        port = toml["port"]
    else
        port = 1883
    end

    if "user" in ks
        user = toml["user"]
        passwd = toml["password"]
    else
        user = ""
        passwd = ""
    end

    uu = User(user, passwd)
    if "rpm" in ks
        topic_rpm = toml["rpm"]
    else
        topic_rpm = name * "/rpm"
    end

    if "status" in ks
        topic_status = toml["status"]
    else
        topic_status = name * "/status"
    end

    client = reconnect!(broker, port, uu)

    return TunelRPM(name, broker, port, user, passwd, topic_rpm, topic_status,
                    client, (0, false))
end

function reconnect!(broker, port, user)

    if user.name == ""
        client, conn = MakeConnection(broker, port)
    else
        client, conn = MakeConnection(broker, port; user=user)
    end
    
    connect(client, conn)
    return client
end

function reconnect!(t::TunelRPM)
    u = User(t.user, t.passwd)
    broker = t.broker
    port = t.port
    client = reconnect!(broker, port, user)
    t.client = client
end


function Base.show(io::IO, v::TunelRPM) ## ,Função para mostrar as informações do ventilador
    println(io, "Nome : ", v.name) 
    println(io, "IP broker   : ", v.broker)
    println(io, "Porta       : ", v.port)
    println(io, "Topico RPM  : ", v.topic_rpm)
    println(io, "Topico Status: ", v.topic_status)  
end
           
function run!(tunel::TunelRPM)
    MQTTClient.publish(tunel.client, tunel.topic_status, "run"; qos=QOS_2)
    tunel.status = (45, true)
    
end


function stop!(tunel::TunelRPM)
   MQTTClient.publish(tunel.client, tunel.topic_status, "stop"; qos=QOS_2)
    tunel.status = (0, false)
end
function sendRPM!(tunel::TunelRPM, rpm)
    if tunel.status[2]
        MQTTClient.publish(tunel.client, tunel.topic_rpm, string(rpm); qos=QOS_2)
        tunel.status = (rpm, true)
    end
    
    
end

# Implementar a API do DAQCore

import DAQCore
DAQCore.devname(tunel::TunelRPM) = tunel.name
DAQCore.devtype(tunel::TunelRPM) = "TunelRPM"
DAQCore.numaxes(tunel::TunelRPM) = 1
DAQCore.axesnames(tunel::TunelRPM) = ["rpm"]

function DAQCore.moveto!(tunel::TunelRPM, rpm)

    rold, status = tunel.status

    if !status && rpm > 0
        run!(tunel)
    end
    st = true
    
    if rpm == 0
        stop!(tunel)
        st = false
    else
        sendRPM!(tunel, rpm)
    end
    
    tunel.status = (rpm, st)
    
end


DAQCore.devposition(tunel::TunelRPM) = tunel.status[1]

        
    
    

end #module Ventilador
