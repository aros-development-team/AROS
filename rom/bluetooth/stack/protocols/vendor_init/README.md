# `protocols/vendor_init/` — bring-up de firmware vendor-specific

Cada subpasta implementa `struct bt_vendor_init_ops` (`include/bluetooth/vendor_init.h`)
para um fabricante de chipset Bluetooth que exige upload de firmware antes de se comportar
como um controlador HCI padrão. Contexto completo da pesquisa, com citação de
arquivo/linha real, em
[`../../ai-context/deteccao-adaptador-firmware-propostas.md`](../../ai-context/deteccao-adaptador-firmware-propostas.md).

## Contrato

* Um módulo **nunca** empacota bytes de firmware no repositório — recebe `firmware_data`/
  `firmware_length` já lidos pela camada de porte a partir do filesystem do host (ver seção
  de licenciamento abaixo).
* `run()` só fala com o controlador através do `bt_hci_transport` já aberto, com comandos
  HCI vendor-specific normais (OGF 0x3F) — nenhum acesso cru a USB/UART/pipe é necessário,
  porque é assim que todo fabricante conhecido implementa esse upload de verdade.
* `matches_usb_id()` é só um auxiliar para portas USB decidirem qual módulo chamar a partir
  de `idVendor`/`idProduct`; portas não-USB (ex. chip onboard por UART) identificam o chip
  do jeito que fizer sentido para aquele barramento e chamam o módulo certo diretamente.
* Ver [`dummy/`](dummy/) para o exemplo mínimo funcional do contrato (nenhum firmware
  necessário, `run()` só retorna `BT_OK`) — comece por ali para ver a forma exata esperada
  antes de implementar um módulo real.

## Licenciamento — cuidado ao implementar um módulo real

Os opcodes HCI vendor-specific citados nos READMEs de cada módulo são fatos de protocolo
público (não são expressão protegida por direito autoral). O **código** que monta esses
comandos deve ser implementado de forma independente, a partir de datasheets/especificações
públicas — nunca copiado ou traduzido de código de terceiros sob licença incompatível
(BlueZ/Linux são GPL; `project.md` já trata dessa mesma restrição para BTstack/NimBLE).

## Estado atual

Só [`dummy/`](dummy/) está implementado e testado (`tests/vendor_init/`). Os demais são
pontos de partida para quem quiser adicionar suporte a um chipset real — nenhum deles tem
código ainda, só o README com o que já se sabe.
