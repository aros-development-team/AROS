# Intel (`AX200`/`AX201`/`9260`/`9560`/`3168`/`7265` etc.) — não implementado

Ponto de partida para quem for implementar o módulo de bring-up Intel. Ver contrato geral em
[`../README.md`](../README.md) e o exemplo mínimo funcional em [`../dummy/`](../dummy/).

## O que já se sabe (não verificado em hardware real ainda)

* Sequência via comandos HCI vendor-specific normais (OGF 0x3F), pelo mesmo
  `bt_hci_transport` que qualquer outro comando usa — não precisa de acesso cru a USB.
* Opcodes conhecidos publicamente: `0xFC01` ("Intel Reset") seguido de um handshake de
  secure boot (leitura de versão/manifest, depois envio do firmware em fragmentos via
  comandos "Secure Send").
* Um passo de leitura de versão/revisão do chip normalmente precisa acontecer **antes** de
  saber qual arquivo de firmware pedir — o nome do arquivo depende da variante exata de
  hardware (ver questão em aberto #3 de
  `ai-context/deteccao-adaptador-firmware-propostas.md` sobre onde esse passo deveria morar:
  camada portátil ou porte).
* É o fabricante mais comum em notebooks x64 de médio/alto padrão (ver discussão registrada
  na mesma nota de `ai-context/`).

## Não implementado

Nenhum código aqui ainda. Implementar `struct bt_vendor_init_ops` seguindo
`include/bluetooth/vendor_init.h`, com testes que não dependam de hardware real.
