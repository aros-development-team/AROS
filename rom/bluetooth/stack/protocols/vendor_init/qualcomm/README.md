# Qualcomm/Atheros (`QCA61x4`/`QCA9377` etc.) — não implementado

Ponto de partida para quem for implementar o módulo de bring-up Qualcomm/Atheros. Ver
contrato geral em [`../README.md`](../README.md) e o exemplo mínimo funcional em
[`../dummy/`](../dummy/).

## O que já se sabe (não verificado em hardware real ainda)

* Sequência via comandos HCI vendor-specific normais (OGF 0x3F), pelo mesmo
  `bt_hci_transport` que qualquer outro comando usa — não precisa de acesso cru a USB.
* Comum em notebooks Dell/HP de gerações intermediárias (ver discussão registrada em
  `ai-context/deteccao-adaptador-firmware-propostas.md`).
* Variantes UART do mesmo chip usam um comando adicional de troca de baud rate antes do
  download de firmware — não confirmado se algum alvo deste projeto usa a variante UART
  deste fabricante especificamente (distinto do combo Broadcom do Raspberry Pi).

## Não implementado

Nenhum código aqui ainda. Implementar `struct bt_vendor_init_ops` seguindo
`include/bluetooth/vendor_init.h`, com testes que não dependam de hardware real.
