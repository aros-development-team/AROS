# Realtek (`RTL8821CE`/`RTL8822CE`/`RTL8852` etc.) — não implementado

Ponto de partida para quem for implementar o módulo de bring-up Realtek. Ver contrato geral
em [`../README.md`](../README.md) e o exemplo mínimo funcional em [`../dummy/`](../dummy/).

## O que já se sabe (não verificado em hardware real ainda)

* Sequência via comandos HCI vendor-specific normais (OGF 0x3F), pelo mesmo
  `bt_hci_transport` que qualquer outro comando usa — não precisa de acesso cru a USB.
* Opcode conhecido publicamente: `0xFC20` e variantes, seguindo o protocolo de patch
  conhecido como "RTL_EPATCH".
* Hoje é o fabricante dominante em notebooks x64 de entrada/OEM baratos (ver discussão
  registrada em `ai-context/deteccao-adaptador-firmware-propostas.md`).

## Não implementado

Nenhum código aqui ainda. Implementar `struct bt_vendor_init_ops` seguindo
`include/bluetooth/vendor_init.h`, com testes que não dependam de hardware real.
