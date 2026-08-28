# Broadcom/Cypress (`BCM20702`, `BCM4345`/`BCM43438` etc.) — não implementado

Ponto de partida para quem for implementar o módulo de bring-up Broadcom. Ver contrato geral
em [`../README.md`](../README.md) e o exemplo mínimo funcional em [`../dummy/`](../dummy/).

## O que já se sabe (não verificado em hardware real ainda)

* Sequência via comandos HCI vendor-specific normais (OGF 0x3F), pelo mesmo
  `bt_hci_transport` que qualquer outro comando usa — não precisa de acesso cru a USB/UART.
* Opcodes conhecidos publicamente: `0xFC2E` ("Download Minidriver") e `0xFC4C` ("Launch
  RAM"). O firmware é enviado em fragmentos como parâmetro desses comandos.
* Esse é exatamente o chip do combo Wi-Fi+Bluetooth do Raspberry Pi (`BCM4345`/`BCM43438`)
  — ali o transporte é UART, não USB, mas o protocolo de upload é o mesmo (ver
  `ai-context/deteccao-adaptador-firmware-propostas.md`).
* Convenção de nome/local de arquivo de firmware ainda não definida para este projeto —
  Linux usa `.hcd` sob `/lib/firmware/brcm/`; algo equivalente sob
  `SYS:Firmware/bluetooth/broadcom/` é a proposta em aberto.

## Não implementado

Nenhum código aqui ainda. Implementar `struct bt_vendor_init_ops` seguindo
`include/bluetooth/vendor_init.h`, com testes que não dependam de hardware real (vetores de
bytes fixos, como o resto do projeto faz para HCI/L2CAP/SDP/etc.).
