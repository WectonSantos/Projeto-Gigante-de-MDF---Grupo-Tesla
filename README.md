🛠️ Gigante de MDF – Projeto de Programação de Hardware 2025 - Grupo Tesla \n

Projeto desenvolvido para a disciplina Programação de Hardware, utilizando o microcontrolador ATMega328(P/PB) e envolvendo mecânica, eletrônica e firmware embarcado.

📌 Sobre o Projeto
O objetivo deste trabalho foi aplicar, na prática, os conteúdos estudados ao longo do semestre, criando um robô móvel (“Gigante de MDF”) capaz de:

• Locomover-se por meio de motores controlados via PWM
• Emitir feixes de laser vermelho (≤ 300 mW)
• Detectar impactos recebidos por lasers adversários
• Gerenciar um sistema de “vidas” por LEDs
• Realizar reações automáticas ao ser atingido

O foco principal do projeto foi o desenvolvimento do firmware, sem depender de shields prontos, bibliotecas externas não autorizadas ou soluções prontas da comunidade Arduino.

🚗 Estrutura Mecânica
O robô construído segue o conjunto de requisitos mecânicos:

• Dimensões máximas de 200 × 200 × 200 mm
• Sistema com 2 a 4 rodas, capaz de realizar movimentação no próprio eixo
• “Arma” laser posicionada no centro do robô, a 100 mm de altura
• Receptor de luz instalado ao lado direito da arma, com difusor de 20 mm
• Estrutura baseada em MDF, com cortes e montagem conforme projeto disponibilizado

🧠 Firmware e Programação
O firmware foi desenvolvido inteiramente do zero, com base no datasheet oficial da Microchip, sem bibliotecas prontas (exceto as eventualmente autorizadas pelo professor).
O código atende às seguintes especificações:

✔️ Microcontrolador
• ATMega328P
✔️ Requisitos de Programação
• Controle dos motores via PWM de hardware
• Laser temporizado por Timer
• Receptor de luz com rotina de interrupção
• Delay de segurança de 5s após cada acerto
• Gestão de LEDs e lógica de “vidas”
• Rotina de giro automático de 180°
• Reset de vidas via interrupção externa
• Código completamente documentado em Doxygen

📚 Documentação
A documentação completa do software foi gerada usando Doxygen e está disponível em:
/documentacao/doxygen/

👥 Integrantes – Grupo Tesla
• Rafael Wippich
• Thiago Taveira
• Wecton Santos
