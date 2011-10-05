/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(NOVUEAU_PCIIDS_H)
#define NOVUEAU_PCIIDS_H

#include "drmP.h"

struct drm_pciid nouveau_pciids [] =
{
    { 0x10de, 0x0020 }, /* NV4 [RIVA TNT] */
    { 0x10de, 0x0028 }, /* NV5 [RIVA TNT2/TNT2 Pro] */
    { 0x10de, 0x0029 }, /* NV5 [RIVA TNT2 Ultra] */
    { 0x10de, 0x002a }, /* NV5 [Riva TNT2] */
    { 0x10de, 0x002b }, /* NV5 [Riva TNT2] */
    { 0x10de, 0x002c }, /* NV6 [Vanta/Vanta LT] */
    { 0x10de, 0x002d }, /* NV5M64 [RIVA TNT2 Model 64/Model 64 Pro] */
    { 0x10de, 0x002e }, /* NV6 [Vanta] */
    { 0x10de, 0x002f }, /* NV6 [Vanta] */
    { 0x10de, 0x0040 }, /* NV40 [GeForce 6800 Ultra] */
    { 0x10de, 0x0041 }, /* NV40 [GeForce 6800] */
    { 0x10de, 0x0042 }, /* NV40.2 [GeForce 6800 LE] */
    { 0x10de, 0x0043 }, /* NV40.3 [GeForce 6800 XE] */
    { 0x10de, 0x0044 }, /* NV40 [GeForce 6800 XT] */
    { 0x10de, 0x0045 }, /* NV40 [GeForce 6800 GT] */
    { 0x10de, 0x0046 }, /* NV45 [GeForce 6800 GT] */
    { 0x10de, 0x0047 }, /* NV40 [GeForce 6800 GS] */
    { 0x10de, 0x0048 }, /* NV40 [GeForce 6800 XT] */
    { 0x10de, 0x0049 }, /* NV40GL */
    { 0x10de, 0x004d }, /* NV40GL [Quadro FX 4000] */
    { 0x10de, 0x004e }, /* NV40GL [Quadro FX 4000] */
    { 0x10de, 0x0090 }, /* G70 [GeForce 7800 GTX] */
    { 0x10de, 0x0091 }, /* G70 [GeForce 7800 GTX] */
    { 0x10de, 0x0092 }, /* G70 [GeForce 7800 GT] */
    { 0x10de, 0x0093 }, /* G70 [GeForce 7800 GS] */
    { 0x10de, 0x0095 }, /* G70 [GeForce 7800 SLI] */
    { 0x10de, 0x0098 }, /* G70 [GeForce Go 7800] */
    { 0x10de, 0x0099 }, /* G70 [GeForce Go 7800 GTX] */
    { 0x10de, 0x009d }, /* G70GL [Quadro FX 4500] */
    { 0x10de, 0x00a0 }, /* NV5 [Aladdin TNT2] */
    { 0x10de, 0x00c0 }, /* NV41 [GeForce 6800 GS] */
    { 0x10de, 0x00c1 }, /* NV41.1 [GeForce 6800] */
    { 0x10de, 0x00c2 }, /* NV41.2 [GeForce 6800 LE] */
    { 0x10de, 0x00c3 }, /* NV42 [GeForce 6800 XT] */
    { 0x10de, 0x00c8 }, /* NV41.8 [GeForce Go 6800] */
    { 0x10de, 0x00c9 }, /* NV41.9 [GeForce Go 6800 Ultra] */
    { 0x10de, 0x00cc }, /* NV41 [Quadro FX Go1400] */
    { 0x10de, 0x00cd }, /* NV41 [Quadro FX 3450/4000 SDI] */
    { 0x10de, 0x00ce }, /* NV41GL [Quadro FX 1400] */
    { 0x10de, 0x00f0 }, /* NV40 [GeForce 6800 Ultra] */
    { 0x10de, 0x00f1 }, /* NV43 [GeForce 6600 GT] */
    { 0x10de, 0x00f2 }, /* NV43 [GeForce 6600] */
    { 0x10de, 0x00f3 }, /* NV43 [GeForce 6200] */
    { 0x10de, 0x00f4 }, /* NV43 [GeForce 6600 LE] */
    { 0x10de, 0x00f5 }, /* G70 [GeForce 7800 GS] */
    { 0x10de, 0x00f6 }, /* NV43 [GeForce 6800 GS] */
    { 0x10de, 0x00f8 }, /* NV45GL [Quadro FX 3400/4400] */
    { 0x10de, 0x00f9 }, /* NV45 [GeForce 6800 GTO] */
    { 0x10de, 0x00fa }, /* NV36 [GeForce PCX 5750] */
    { 0x10de, 0x00fb }, /* NV35 [GeForce PCX 5900] */
    { 0x10de, 0x00fc }, /* NV37GL [Quadro FX 330/GeForce PCX 5300] */
    { 0x10de, 0x00fd }, /* NV37GL [Quadro PCI-E Series] */
    { 0x10de, 0x00fe }, /* NV38GL [Quadro FX 1300] */
    { 0x10de, 0x00ff }, /* NV18 [GeForce PCX 4300] */
    { 0x10de, 0x0100 }, /* NV10 [GeForce 256 SDR] */
    { 0x10de, 0x0101 }, /* NV10DDR [GeForce 256 DDR] */
    { 0x10de, 0x0103 }, /* NV10GL [Quadro] */
    { 0x10de, 0x0110 }, /* NV11 [GeForce2 MX/MX 400] */
    { 0x10de, 0x0111 }, /* NV11DDR [GeForce2 MX200] */
    { 0x10de, 0x0112 }, /* NV11 [GeForce2 Go] */
    { 0x10de, 0x0113 }, /* NV11GL [Quadro2 MXR/EX/Go] */
    { 0x10de, 0x0140 }, /* NV43 [GeForce 6600 GT] */
    { 0x10de, 0x0141 }, /* NV43 [GeForce 6600] */
    { 0x10de, 0x0142 }, /* NV43 [GeForce 6600 LE] */
    { 0x10de, 0x0143 }, /* NV43 [GeForce 6600 VE] */
    { 0x10de, 0x0144 }, /* NV43 [GeForce Go 6600] */
    { 0x10de, 0x0145 }, /* NV43 [GeForce 6610 XL] */
    { 0x10de, 0x0146 }, /* NV43 [GeForce Go 6600TE/6200TE] */
    { 0x10de, 0x0147 }, /* NV43 [GeForce 6700 XL] */
    { 0x10de, 0x0148 }, /* NV43 [GeForce Go 6600] */
    { 0x10de, 0x0149 }, /* NV43 [GeForce Go 6600 GT] */
    { 0x10de, 0x014a }, /* NV43 [Quadro NVS 440] */
    { 0x10de, 0x014c }, /* NV43 [Quadro FX 540 MXM] */
    { 0x10de, 0x014d }, /* NV43GL [Quadro FX 550] */
    { 0x10de, 0x014e }, /* NV43GL [Quadro FX 540] */
    { 0x10de, 0x014f }, /* NV43 [GeForce 6200] */
    { 0x10de, 0x0150 }, /* NV15 [GeForce2 GTS/Pro] */
    { 0x10de, 0x0151 }, /* NV15DDR [GeForce2 Ti] */
    { 0x10de, 0x0152 }, /* NV15BR [GeForce2 Ultra, Bladerunner] */
    { 0x10de, 0x0153 }, /* NV15GL [Quadro2 Pro] */
    { 0x10de, 0x0160 }, /* NV44 [GeForce 6500] */
    { 0x10de, 0x0161 }, /* NV44 [GeForce 6200 TurboCache(TM)] */
    { 0x10de, 0x0162 }, /* NV44 [GeForce 6200SE TurboCache (TM)] */
    { 0x10de, 0x0163 }, /* NV44 [GeForce 6200 LE] */
    { 0x10de, 0x0164 }, /* NV44 [GeForce Go 6200] */
    { 0x10de, 0x0165 }, /* NV44 [Quadro NVS 285] */
    { 0x10de, 0x0166 }, /* NV44 [GeForce Go 6400] */
    { 0x10de, 0x0167 }, /* NV44 [GeForce Go 6200] */
    { 0x10de, 0x0168 }, /* NV44 [GeForce Go 6400] */
    { 0x10de, 0x0169 }, /* NV44 [GeForce 6250] */
    { 0x10de, 0x016a }, /* NV44 [GeForce 7100 GS] */
    { 0x10de, 0x0170 }, /* NV17 [GeForce4 MX 460] */
    { 0x10de, 0x0171 }, /* NV17 [GeForce4 MX 440] */
    { 0x10de, 0x0172 }, /* NV17 [GeForce4 MX 420] */
    { 0x10de, 0x0173 }, /* NV17 [GeForce4 MX 440-SE] */
    { 0x10de, 0x0174 }, /* NV17 [GeForce4 440 Go] */
    { 0x10de, 0x0175 }, /* NV17 [GeForce4 420 Go] */
    { 0x10de, 0x0176 }, /* NV17 [GeForce4 420 Go 32M] */
    { 0x10de, 0x0177 }, /* NV17 [GeForce4 460 Go] */
    { 0x10de, 0x0178 }, /* NV17GL [Quadro4 550 XGL] */
    { 0x10de, 0x0179 }, /* NV17 [GeForce4 440 Go 64M] */
    { 0x10de, 0x017a }, /* NV17GL [Quadro NVS] */
    { 0x10de, 0x017b }, /* NV17GL [Quadro4 550 XGL] */
    { 0x10de, 0x017c }, /* NV17GL [Quadro4 500 GoGL] */
    { 0x10de, 0x017d }, /* NV17 [GeForce4 410 Go 16M] */
    { 0x10de, 0x0181 }, /* NV18 [GeForce4 MX 440 AGP 8x] */
    { 0x10de, 0x0182 }, /* NV18 [GeForce4 MX 440SE AGP 8x] */
    { 0x10de, 0x0183 }, /* NV18 [GeForce4 MX 420 AGP 8x] */
    { 0x10de, 0x0184 }, /* NV18 [GeForce4 MX] */
    { 0x10de, 0x0185 }, /* NV18 [GeForce4 MX 4000] */
    { 0x10de, 0x0186 }, /* NV18M [GeForce4 448 Go] */
    { 0x10de, 0x0187 }, /* NV18M [GeForce4 488 Go] */
    { 0x10de, 0x0188 }, /* NV18GL [Quadro4 580 XGL] */
    { 0x10de, 0x0189 }, /* NV18 [GeForce4 MX with AGP8X (Mac)] */
    { 0x10de, 0x018a }, /* NV18GL [Quadro NVS 280 SD] */
    { 0x10de, 0x018b }, /* NV18GL [Quadro4 380 XGL] */
    { 0x10de, 0x018c }, /* NV18GL [Quadro NVS 50 PCI] */
    { 0x10de, 0x018d }, /* NV18M [GeForce4 448 Go] */
    { 0x10de, 0x0191 }, /* G80 [GeForce 8800 GTX] */
    { 0x10de, 0x0193 }, /* G80 [GeForce 8800 GTS] */
    { 0x10de, 0x0194 }, /* G80 [GeForce 8800 Ultra] */
    { 0x10de, 0x0197 }, /* G80 [Tesla C870] */
    { 0x10de, 0x019d }, /* G80 [Quadro FX 5600] */
    { 0x10de, 0x019e }, /* G80 [Quadro FX 4600] */
    { 0x10de, 0x01a0 }, /* NVCrush11 [GeForce2 MX Integrated Graphics] */
    { 0x10de, 0x01d0 }, /* G72 [GeForce 7350 LE] */
    { 0x10de, 0x01d1 }, /* G72 [GeForce 7300 LE] */
    { 0x10de, 0x01d2 }, /* G72 [GeForce 7550 LE] */
    { 0x10de, 0x01d3 }, /* G72 [GeForce 7300 SE/7200 GS] */
    { 0x10de, 0x01d6 }, /* G72M [GeForce Go 7200] */
    { 0x10de, 0x01d7 }, /* G72M [Quadro NVS 110M/GeForce Go 7300] */
    { 0x10de, 0x01d8 }, /* G72M [GeForce Go 7400] */
    { 0x10de, 0x01d9 }, /* G72M [GeForce Go 7450] */
    { 0x10de, 0x01da }, /* G72M [Quadro NVS 110M] */
    { 0x10de, 0x01db }, /* G72M [Quadro NVS 120M] */
    { 0x10de, 0x01dc }, /* G72GL [Quadro FX 350M] */
    { 0x10de, 0x01dd }, /* G72 [GeForce 7500 LE] */
    { 0x10de, 0x01de }, /* G72GL [Quadro FX 350] */
    { 0x10de, 0x01df }, /* G71 [GeForce 7300 GS] */
    { 0x10de, 0x01e0 }, /* nForce2 IGP2 */
    { 0x10de, 0x01f0 }, /* NV18 [GeForce4 MX - nForce GPU] */
    { 0x10de, 0x0200 }, /* NV20 [GeForce3] */
    { 0x10de, 0x0201 }, /* NV20 [GeForce3 Ti 200] */
    { 0x10de, 0x0202 }, /* NV20 [GeForce3 Ti 500] */
    { 0x10de, 0x0203 }, /* NV20DCC [Quadro DCC] */
    { 0x10de, 0x0211 }, /* NV48 [GeForce 6800] */
    { 0x10de, 0x0212 }, /* NV48 [GeForce 6800 LE] */
    { 0x10de, 0x0215 }, /* NV48 [GeForce 6800 GT] */
    { 0x10de, 0x0218 }, /* NV48 [GeForce 6800 XT] */
    { 0x10de, 0x0221 }, /* NV44A [GeForce 6200] */
    { 0x10de, 0x0222 }, /* NV44 [GeForce 6200 A-LE] */
    { 0x10de, 0x0240 }, /* C51PV [GeForce 6150] */
    { 0x10de, 0x0241 }, /* C51 [GeForce 6150 LE] */
    { 0x10de, 0x0242 }, /* C51G [GeForce 6100] */
    { 0x10de, 0x0244 }, /* C51 [GeForce Go 6150] */
    { 0x10de, 0x0245 }, /* C51 [Quadro NVS 210S/GeForce 6150LE] */
    { 0x10de, 0x0247 }, /* C51 [GeForce Go 6100] */
    { 0x10de, 0x0250 }, /* NV25 [GeForce4 Ti 4600] */
    { 0x10de, 0x0251 }, /* NV25 [GeForce4 Ti 4400] */
    { 0x10de, 0x0252 }, /* NV25 [GeForce4 Ti] */
    { 0x10de, 0x0253 }, /* NV25 [GeForce4 Ti 4200] */
    { 0x10de, 0x0258 }, /* NV25GL [Quadro4 900 XGL] */
    { 0x10de, 0x0259 }, /* NV25GL [Quadro4 750 XGL] */
    { 0x10de, 0x025b }, /* NV25GL [Quadro4 700 XGL] */
    { 0x10de, 0x0280 }, /* NV28 [GeForce4 Ti 4800] */
    { 0x10de, 0x0281 }, /* NV28 [GeForce4 Ti 4200 AGP 8x] */
    { 0x10de, 0x0282 }, /* NV28 [GeForce4 Ti 4800 SE] */
    { 0x10de, 0x0286 }, /* NV28 [GeForce4 Ti 4200 Go AGP 8x] */
    { 0x10de, 0x0288 }, /* NV28GL [Quadro4 980 XGL] */
    { 0x10de, 0x0289 }, /* NV28GL [Quadro4 780 XGL] */
    { 0x10de, 0x028c }, /* NV28GLM [Quadro4 Go700] */
    { 0x10de, 0x0290 }, /* G71 [GeForce 7900 GTX] */
    { 0x10de, 0x0291 }, /* G71 [GeForce 7900 GT/GTO] */
    { 0x10de, 0x0292 }, /* G71 [GeForce 7900 GS] */
    { 0x10de, 0x0293 }, /* G71 [GeForce 7900 GX2] */
    { 0x10de, 0x0294 }, /* G71 [GeForce 7950 GX2] */
    { 0x10de, 0x0295 }, /* G71 [GeForce 7950 GT] */
    { 0x10de, 0x0297 }, /* G71 [GeForce Go 7950 GTX] */
    { 0x10de, 0x0298 }, /* G71 [GeForce Go 7900 GS] */
    { 0x10de, 0x0299 }, /* G71 [GeForce Go 7900 GTX] */
    { 0x10de, 0x029a }, /* G71 [Quadro FX 2500M] */
    { 0x10de, 0x029b }, /* G71 [Quadro FX 1500M] */
    { 0x10de, 0x029c }, /* G71 [Quadro FX 5500] */
    { 0x10de, 0x029d }, /* G71GL [Quadro FX 3500] */
    { 0x10de, 0x029e }, /* G71 [Quadro FX 1500] */
    { 0x10de, 0x029f }, /* G70 [Quadro FX 4500 X2] */
    { 0x10de, 0x02a0 }, /* NV2A [XGPU] */
    { 0x10de, 0x02e0 }, /* G73 [GeForce 7600 GT] */
    { 0x10de, 0x02e1 }, /* G73 [GeForce 7600 GS] */
    { 0x10de, 0x02e2 }, /* G73 [GeForce 7300 GT] */
    { 0x10de, 0x02e3 }, /* G71 [GeForce 7900 GS] */
    { 0x10de, 0x02e4 }, /* G71 [GeForce 7950 GT] */
    { 0x10de, 0x0300 }, /* NV30 [GeForce FX] */
    { 0x10de, 0x0301 }, /* NV30 [GeForce FX 5800 Ultra] */
    { 0x10de, 0x0302 }, /* NV30 [GeForce FX 5800] */
    { 0x10de, 0x0308 }, /* NV30GL [Quadro FX 2000] */
    { 0x10de, 0x0309 }, /* NV30GL [Quadro FX 1000] */
    { 0x10de, 0x0311 }, /* NV31 [GeForce FX 5600 Ultra] */
    { 0x10de, 0x0312 }, /* NV31 [GeForce FX 5600] */
    { 0x10de, 0x0313 }, /* NV31 */
    { 0x10de, 0x0314 }, /* NV31 [GeForce FX 5600XT] */
    { 0x10de, 0x0316 }, /* NV31M */
    { 0x10de, 0x0317 }, /* NV31M Pro */
    { 0x10de, 0x031a }, /* NV31M [GeForce FX Go5600] */
    { 0x10de, 0x031b }, /* NV31M [GeForce FX Go5650] */
    { 0x10de, 0x031c }, /* NV31 [Quadro FX Go700] */
    { 0x10de, 0x031d }, /* NV31GLM */
    { 0x10de, 0x031e }, /* NV31GLM Pro */
    { 0x10de, 0x031f }, /* NV31GLM Pro */
    { 0x10de, 0x0320 }, /* NV34 [GeForce FX 5200] */
    { 0x10de, 0x0321 }, /* NV34 [GeForce FX 5200 Ultra] */
    { 0x10de, 0x0322 }, /* NV34 [GeForce FX 5200] */
    { 0x10de, 0x0323 }, /* NV34 [GeForce FX 5200LE] */
    { 0x10de, 0x0324 }, /* NV34M [GeForce FX Go5200 64M] */
    { 0x10de, 0x0325 }, /* NV34M [GeForce FX Go5250] */
    { 0x10de, 0x0326 }, /* NV34 [GeForce FX 5500] */
    { 0x10de, 0x0327 }, /* NV34 [GeForce FX 5100] */
    { 0x10de, 0x0328 }, /* NV34M [GeForce FX Go5200 32M/64M] */
    { 0x10de, 0x0329 }, /* NV34M [GeForce FX Go5200] */
    { 0x10de, 0x032a }, /* NV34GL [Quadro NVS 280 PCI] */
    { 0x10de, 0x032b }, /* NV34GL [Quadro FX 500/600 PCI] */
    { 0x10de, 0x032c }, /* NV34GLM [GeForce FX Go 5300] */
    { 0x10de, 0x032d }, /* NV34 [GeForce FX Go5100] */
    { 0x10de, 0x032f }, /* NV34GL */
    { 0x10de, 0x0330 }, /* NV35 [GeForce FX 5900 Ultra] */
    { 0x10de, 0x0331 }, /* NV35 [GeForce FX 5900] */
    { 0x10de, 0x0332 }, /* NV35 [GeForce FX 5900XT] */
    { 0x10de, 0x0333 }, /* NV38 [GeForce FX 5950 Ultra] */
    { 0x10de, 0x0334 }, /* NV35 [GeForce FX 5900ZT] */
    { 0x10de, 0x0338 }, /* NV35GL [Quadro FX 3000] */
    { 0x10de, 0x033f }, /* NV35GL [Quadro FX 700] */
    { 0x10de, 0x0341 }, /* NV36.1 [GeForce FX 5700 Ultra] */
    { 0x10de, 0x0342 }, /* NV36.2 [GeForce FX 5700] */
    { 0x10de, 0x0343 }, /* NV36 [GeForce FX 5700LE] */
    { 0x10de, 0x0344 }, /* NV36.4 [GeForce FX 5700VE] */
    { 0x10de, 0x0345 }, /* NV36.5 */
    { 0x10de, 0x0347 }, /* NV36 [GeForce FX Go5700] */
    { 0x10de, 0x0348 }, /* NV36 [GeForce FX Go5700] */
    { 0x10de, 0x0349 }, /* NV36M Pro */
    { 0x10de, 0x034b }, /* NV36MAP */
    { 0x10de, 0x034c }, /* NV36 [Quadro FX Go1000] */
    { 0x10de, 0x034e }, /* NV36GL [Quadro FX 1100] */
    { 0x10de, 0x034f }, /* NV36GL */
    { 0x10de, 0x038b }, /* G73 [GeForce 7650 GS] */
    { 0x10de, 0x0390 }, /* G73 [GeForce 7650 GS] */
    { 0x10de, 0x0391 }, /* G73 [GeForce 7600 GT] */
    { 0x10de, 0x0392 }, /* G73 [GeForce 7600 GS] */
    { 0x10de, 0x0393 }, /* G73 [GeForce 7300 GT] */
    { 0x10de, 0x0394 }, /* G73 [GeForce 7600 LE] */
    { 0x10de, 0x0395 }, /* G73 [GeForce 7300 GT] */
    { 0x10de, 0x0397 }, /* G73 [GeForce Go 7700] */
    { 0x10de, 0x0398 }, /* G73 [GeForce Go 7600] */
    { 0x10de, 0x0399 }, /* G73 [GeForce Go 7600 GT] */
    { 0x10de, 0x039a }, /* G73M [Quadro NVS 300M] */
    { 0x10de, 0x039b }, /* G73 [GeForce Go 7900 SE] */
    { 0x10de, 0x039c }, /* G73 [Quadro FX 550M] */
    { 0x10de, 0x039e }, /* G73GL [Quadro FX 560] */
    { 0x10de, 0x03d0 }, /* C61 [GeForce 6150SE nForce 430] */
    { 0x10de, 0x03d1 }, /* C61 [GeForce 6100 nForce 405] */
    { 0x10de, 0x03d2 }, /* C61 [GeForce 6100 nForce 400] */
    { 0x10de, 0x03d5 }, /* C61 [GeForce 6100 nForce 420] */
    { 0x10de, 0x03d6 }, /* C61 [GeForce 7025 / nForce 630a] */
    { 0x10de, 0x0400 }, /* G84 [GeForce 8600 GTS] */
    { 0x10de, 0x0401 }, /* G84 [GeForce 8600GT] */
    { 0x10de, 0x0402 }, /* G84 [GeForce 8600 GT] */
    { 0x10de, 0x0403 }, /* G84 [GeForce 8600 GS] */
    { 0x10de, 0x0404 }, /* G84 [GeForce 8400 GS] */
    { 0x10de, 0x0405 }, /* G84 [GeForce 9500M GS] */
    { 0x10de, 0x0406 }, /* G84 [GeForce 8300 GS] */
    { 0x10de, 0x0407 }, /* G84 [GeForce 8600M GT] */
    { 0x10de, 0x0408 }, /* G84 [GeForce 9650M GS] */
    { 0x10de, 0x0409 }, /* G84 [GeForce 8700M GT] */
    { 0x10de, 0x040a }, /* G84 [Quadro FX 370] */
    { 0x10de, 0x040b }, /* G84M [Quadro NVS 320M] */
    { 0x10de, 0x040c }, /* G84M [Quadro FX 570M] */
    { 0x10de, 0x040d }, /* G84 [Quadro FX 1600M] */
    { 0x10de, 0x040e }, /* G84 [Quadro FX 570] */
    { 0x10de, 0x040f }, /* G84 [Quadro FX 1700] */
    { 0x10de, 0x0410 }, /* G92 [GeForce GT 330] */
    { 0x10de, 0x0420 }, /* G86 [GeForce 8400 SE] */
    { 0x10de, 0x0421 }, /* G86 [GeForce 8500 GT] */
    { 0x10de, 0x0422 }, /* G86 [GeForce 8400 GS] */
    { 0x10de, 0x0423 }, /* G86 [GeForce 8300 GS] */
    { 0x10de, 0x0424 }, /* G86 [GeForce 8400 GS] */
    { 0x10de, 0x0425 }, /* G86 [GeForce 8600M GS] */
    { 0x10de, 0x0426 }, /* G86 [GeForce 8400M GT] */
    { 0x10de, 0x0427 }, /* G86 [GeForce 8400M GS] */
    { 0x10de, 0x0428 }, /* G86 [GeForce 8400M G] */
    { 0x10de, 0x0429 }, /* G84M [Quadro NVS 140M] */
    { 0x10de, 0x042a }, /* G86M [Quadro NVS 130M] */
    { 0x10de, 0x042b }, /* G86M [Quadro NVS 135M] */
    { 0x10de, 0x042c }, /* G86 [GeForce 9400 GT] */
    { 0x10de, 0x042d }, /* G86M [Quadro FX 360M] */
    { 0x10de, 0x042e }, /* G86 [GeForce 9300M G] */
    { 0x10de, 0x042f }, /* G86 [Quadro NVS 290] */
    { 0x10de, 0x0531 }, /* C67 [GeForce 7150M / nForce 630M] */
    { 0x10de, 0x0533 }, /* C67 [GeForce 7000M / nForce 610M] */
    { 0x10de, 0x053a }, /* C68 [GeForce 7050 PV / nForce 630a] */
    { 0x10de, 0x053b }, /* C68 [GeForce 7050 PV / nForce 630a] */
    { 0x10de, 0x053e }, /* C68 [GeForce 7025 / nForce 630a] */
    { 0x10de, 0x05e0 }, /* GT200b [GeForce GTX 295] */
    { 0x10de, 0x05e1 }, /* GT200 [GeForce GTX 280] */
    { 0x10de, 0x05e2 }, /* GT200 [GeForce GTX 260] */
    { 0x10de, 0x05e3 }, /* GT200b [GeForce GTX 285] */
    { 0x10de, 0x05e6 }, /* GT200b [GeForce GTX 275] */
    { 0x10de, 0x05e7 }, /* GT200 [Tesla C1060] */
    { 0x10de, 0x05ea }, /* GT200 [GeForce GTX 260] */
    { 0x10de, 0x05eb }, /* GT200 [GeForce GTX 295] */
    { 0x10de, 0x05ed }, /* GT200GL [Quadro Plex 2200 D2] */
    { 0x10de, 0x05f8 }, /* GT200GL [Quadro Plex 2200 S4] */
    { 0x10de, 0x05f9 }, /* GT200GL [Quadro CX] */
    { 0x10de, 0x05fd }, /* GT200GL [Quadro FX 5800] */
    { 0x10de, 0x05fe }, /* GT200GL [Quadro FX 4800] */
    { 0x10de, 0x05ff }, /* GT200GL [NVIDIA Quadro FX 3800] */
    { 0x10de, 0x0600 }, /* G92 [GeForce 8800 GTS 512] */
    { 0x10de, 0x0601 }, /* G92 [GeForce 9800 GT] */
    { 0x10de, 0x0602 }, /* G92 [GeForce 8800 GT] */
    { 0x10de, 0x0603 }, /* G92 [GeForce GT 230] */
    { 0x10de, 0x0604 }, /* G92 [GeForce 9800 GX2] */
    { 0x10de, 0x0605 }, /* G92 [GeForce 9800 GT] */
    { 0x10de, 0x0606 }, /* G92 [GeForce 8800 GS] */
    { 0x10de, 0x0607 }, /* G92 [GeForce GTS 240] */
    { 0x10de, 0x0608 }, /* G92 [GeForce 9800M GTX] */
    { 0x10de, 0x0609 }, /* G92 [GeForce 8800M GTS] */
    { 0x10de, 0x060a }, /* GT200 [GeForce GTX 280M] */
    { 0x10de, 0x060b }, /* G92 [GeForce 9800M GT] */
    { 0x10de, 0x060c }, /* G92 [GeForce 8800M GTX] */
    { 0x10de, 0x060d }, /* G92 [GeForce 8800 GS] */
    { 0x10de, 0x060f }, /* G92 [GeForce GTX 285M] */
    { 0x10de, 0x0610 }, /* G92 [GeForce 9600 GSO] */
    { 0x10de, 0x0611 }, /* G92 [GeForce 8800 GT] */
    { 0x10de, 0x0612 }, /* G92 [GeForce 9800 GTX] */
    { 0x10de, 0x0613 }, /* G92 [GeForce 9800 GTX+] */
    { 0x10de, 0x0614 }, /* G92 [GeForce 9800 GT] */
    { 0x10de, 0x0615 }, /* G92 [GeForce GTS 250] */
    { 0x10de, 0x0617 }, /* G92 [GeForce 9800M GTX] */
    { 0x10de, 0x0618 }, /* G92 [GeForce GTX 260M] */
    { 0x10de, 0x0619 }, /* G92GL [Quadro FX 4700 X2] */
    { 0x10de, 0x061a }, /* G92 [Quadro FX 3700] */
    { 0x10de, 0x061b }, /* G92GL [Quadro VX 200] */
    { 0x10de, 0x061c }, /* G92M [Quadro FX 3600M] */
    { 0x10de, 0x061d }, /* G92 [Quadro FX 2800M] */
    { 0x10de, 0x061e }, /* G92 [Quadro FX 3700M] */
    { 0x10de, 0x061f }, /* G92 [Quadro FX 3800M] */
    { 0x10de, 0x0622 }, /* G94 [GeForce 9600 GT] */
    { 0x10de, 0x0623 }, /* G94 [GeForce 9600 GS] */
    { 0x10de, 0x0625 }, /* G94 [GeForce 9600 GSO 512] */
    { 0x10de, 0x0626 }, /* G94 [GeForce GT 130] */
    { 0x10de, 0x0627 }, /* G94 [GeForce GT 140] */
    { 0x10de, 0x0628 }, /* G94 [GeForce 9800M GTS] */
    { 0x10de, 0x062a }, /* G94 [GeForce 9700M GTS] */
    { 0x10de, 0x062b }, /* G94 [GeForce 9800M GS] */
    { 0x10de, 0x062c }, /* G94 [GeForce 9800M GTS] */
    { 0x10de, 0x062d }, /* G94 [GeForce 9600 GT] */
    { 0x10de, 0x062e }, /* G94 [GeForce 9600 GT] */
    { 0x10de, 0x0631 }, /* G94M [GeForce GTS 160M] */
    { 0x10de, 0x0632 }, /* G94M [GeForce GTS 150M] */
    { 0x10de, 0x0635 }, /* G94 [GeForce 9600 GSO] */
    { 0x10de, 0x0637 }, /* G94 [GeForce 9600 GT] */
    { 0x10de, 0x0638 }, /* G94 [Quadro FX 1800] */
    { 0x10de, 0x063a }, /* G94M [Quadro FX 2700M] */
    { 0x10de, 0x0640 }, /* G96 [GeForce 9500 GT] */
    { 0x10de, 0x0641 }, /* G96 [GeForce 9400 GT] */
    { 0x10de, 0x0643 }, /* G96 [GeForce 9500 GT] */
    { 0x10de, 0x0644 }, /* G96 [GeForce 9500 GS] */
    { 0x10de, 0x0645 }, /* G96 [GeForce 9500 GS] */
    { 0x10de, 0x0646 }, /* G96 [GeForce GT 120] */
    { 0x10de, 0x0647 }, /* G96 [GeForce 9600M GT] */
    { 0x10de, 0x0648 }, /* G96 [GeForce 9600M GS] */
    { 0x10de, 0x0649 }, /* G96 [GeForce 9600M GT] */
    { 0x10de, 0x064a }, /* G96 [GeForce 9700M GT] */
    { 0x10de, 0x064b }, /* G96 [GeForce 9500M G] */
    { 0x10de, 0x064c }, /* G96 [GeForce 9650M GT] */
    { 0x10de, 0x0651 }, /* G96 [GeForce G 110M] */
    { 0x10de, 0x0652 }, /* G96 [GeForce GT 130M] */
    { 0x10de, 0x0653 }, /* G96M [GeForce GT 120M] */
    { 0x10de, 0x0654 }, /* G96 [GeForce GT 220M] */
    { 0x10de, 0x0656 }, /* G96 [GeForce 9650 S] */
    { 0x10de, 0x0658 }, /* G96 [Quadro FX 380] */
    { 0x10de, 0x0659 }, /* G96 [Quadro FX 580] */
    { 0x10de, 0x065a }, /* G96 [Quadro FX 1700M] */
    { 0x10de, 0x065b }, /* G96 [GeForce 9400 GT] */
    { 0x10de, 0x065c }, /* G96M [Quadro FX 770M] */
    { 0x10de, 0x06c0 }, /* GF100 [GeForce GTX 480] */
    { 0x10de, 0x06cd }, /* GF100 [GeForce GTX 470] */
    { 0x10de, 0x06d1 }, /* GF100 [Tesla C2050 / C2070] */
    { 0x10de, 0x06d2 }, /* GF100 [M2070] */
    { 0x10de, 0x06d8 }, /* GF100 [Quadro 6000] */
    { 0x10de, 0x06d9 }, /* GF100 [Quadro 5000] */
    { 0x10de, 0x06dd }, /* GF100 [Quadro 4000] */
    { 0x10de, 0x06de }, /* GF100 [Tesla S2050] */
    { 0x10de, 0x06df }, /* GF100 [M2070Q] */
    { 0x10de, 0x06e0 }, /* G98 [GeForce 9300 GE] */
    { 0x10de, 0x06e1 }, /* G98 [GeForce 9300 GS] */
    { 0x10de, 0x06e2 }, /* G98 [GeForce 8400] */
    { 0x10de, 0x06e3 }, /* G98 [GeForce 8300 GS] */
    { 0x10de, 0x06e4 }, /* G98 [GeForce 8400 GS] */
    { 0x10de, 0x06e5 }, /* G98 [GeForce 9300M GS] */
    { 0x10de, 0x06e6 }, /* G98 [GeForce G100] */
    { 0x10de, 0x06e7 }, /* G98 [GeForce 9300 SE] */
    { 0x10de, 0x06e8 }, /* G98 [GeForce 9200M GS] */
    { 0x10de, 0x06e9 }, /* G98 [GeForce 9300M GS] */
    { 0x10de, 0x06ea }, /* G86M [Quadro NVS 150M] */
    { 0x10de, 0x06eb }, /* G98M [Quadro NVS 160M] */
    { 0x10de, 0x06ec }, /* G98M [GeForce G 105M] */
    { 0x10de, 0x06ef }, /* G98M [GeForce G 103M] */
    { 0x10de, 0x06f1 }, /* G98 [GeForce G105M] */
    { 0x10de, 0x06f8 }, /* G98 [Quadro NVS 420] */
    { 0x10de, 0x06f9 }, /* G98 [Quadro FX 370 LP] */
    { 0x10de, 0x06fa }, /* G98 [Quadro NVS 450] */
    { 0x10de, 0x06fb }, /* G98 [Quadro FX 370M] */
    { 0x10de, 0x06fd }, /* G98 [Quadro NVS 295] */
    { 0x10de, 0x06ff }, /* G98 [HICx16 + Graphics] */
    { 0x10de, 0x07e0 }, /* C73 [GeForce 7150 / nForce 630i] */
    { 0x10de, 0x07e1 }, /* C73 [GeForce 7100 / nForce 630i] */
    { 0x10de, 0x07e2 }, /* C73 [GeForce 7050 / nForce 630i] */
    { 0x10de, 0x07e3 }, /* C73 [GeForce 7050 / nForce 610i] */
    { 0x10de, 0x07e5 }, /* C73 [GeForce 7100 / nForce 620i] */
    { 0x10de, 0x0844 }, /* C77 [GeForce 9100M G] */
    { 0x10de, 0x0845 }, /* C77 [GeForce 8200M G] */
    { 0x10de, 0x0846 }, /* C77 [GeForce 9200] */
    { 0x10de, 0x0847 }, /* C78 [GeForce 9100] */
    { 0x10de, 0x0848 }, /* C77 [GeForce 8300] */
    { 0x10de, 0x0849 }, /* C77 [GeForce 8200] */
    { 0x10de, 0x084a }, /* C77 [nForce 730a] */
    { 0x10de, 0x084b }, /* C77 [GeForce 8200] */
    { 0x10de, 0x084c }, /* C77 [nForce 780a SLI] */
    { 0x10de, 0x084d }, /* C77 [nForce 750a SLI] */
    { 0x10de, 0x084f }, /* C77 [GeForce 8100 / nForce 720a] */
    { 0x10de, 0x0860 }, /* C79 [GeForce 9300] */
    { 0x10de, 0x0861 }, /* C79 [GeForce 9400] */
    { 0x10de, 0x0862 }, /* C79 [GeForce 9400M G] */
    { 0x10de, 0x0863 }, /* C79 [GeForce 9400M] */
    { 0x10de, 0x0864 }, /* C79 [GeForce 9300] */
    { 0x10de, 0x0865 }, /* C79 [GeForce 9300] */
    { 0x10de, 0x0866 }, /* C79 [GeForce 9400M G] */
    { 0x10de, 0x0867 }, /* C79 [GeForce 9400] */
    { 0x10de, 0x0868 }, /* C79 [nForce 760i SLI] */
    { 0x10de, 0x086a }, /* C79 [GeForce 9400] */
    { 0x10de, 0x086c }, /* C79 [GeForce 9300 / nForce 730i] */
    { 0x10de, 0x086d }, /* C79 [GeForce 9200] */
    { 0x10de, 0x086e }, /* C79 [GeForce 9100M G] */
    { 0x10de, 0x086f }, /* C79 [GeForce 9200M G] */
    { 0x10de, 0x0870 }, /* C79 [GeForce 9400M] */
    { 0x10de, 0x0871 }, /* C79 [GeForce 9200] */
    { 0x10de, 0x0872 }, /* C79 [GeForce G102M] */
    { 0x10de, 0x0873 }, /* C79 [GeForce G102M] */
    { 0x10de, 0x0874 }, /* C79 [ION] */
    { 0x10de, 0x0876 }, /* ION VGA [GeForce 9400M] */
    { 0x10de, 0x087a }, /* C79 [GeForce 9400] */
    { 0x10de, 0x087d }, /* ION VGA */
    { 0x10de, 0x087e }, /* ION LE VGA */
    { 0x10de, 0x087f }, /* ION LE VGA */
    { 0x10de, 0x08a4 }, /* GT216 [GeForce 320M] */
    { 0x10de, 0x0a20 }, /* GT216 [GeForce GT 220] */
    { 0x10de, 0x0a23 }, /* GT218 [GeForce 210] */
    { 0x10de, 0x0a28 }, /* GT216 [GeForce GT 230M] */
    { 0x10de, 0x0a29 }, /* GT216 [GeForce GT 330M] */
    { 0x10de, 0x0a2a }, /* GT216 [GeForce GT 230M] */
    { 0x10de, 0x0a2b }, /* GT216 [GeForce GT 330M] */
    { 0x10de, 0x0a2c }, /* GT216 [NVS 5100M] */
    { 0x10de, 0x0a2d }, /* GT216 [GeForce GT 320M] */
    { 0x10de, 0x0a34 }, /* GT216 [GeForce GT 240M] */
    { 0x10de, 0x0a35 }, /* GT216 [GeForce GT 325M] */
    { 0x10de, 0x0a38 }, /* GT216GL [Quadro 400] */
    { 0x10de, 0x0a3c }, /* GT216 [Quadro FX 880M] */
    { 0x10de, 0x0a60 }, /* GT218 [GeForce G210] */
    { 0x10de, 0x0a62 }, /* GT218 [GeForce 205] */
    { 0x10de, 0x0a63 }, /* GT218 [GeForce 310] */
    { 0x10de, 0x0a64 }, /* GT218 [ION] */
    { 0x10de, 0x0a65 }, /* GT218 [GeForce 210] */
    { 0x10de, 0x0a66 }, /* GT218 [GeForce 310] */
    { 0x10de, 0x0a68 }, /* G98M [GeForce G105M] */
    { 0x10de, 0x0a69 }, /* G98M [GeForce G105M] */
    { 0x10de, 0x0a6a }, /* GT218 [NVS 2100M] */
    { 0x10de, 0x0a6c }, /* GT218 [NVS 3100M] */
    { 0x10de, 0x0a6e }, /* GT218 [GeForce 305M] */
    { 0x10de, 0x0a6f }, /* GT218 [ION] */
    { 0x10de, 0x0a70 }, /* GT218 [GeForce 310M] */
    { 0x10de, 0x0a71 }, /* GT218 [GeForce 305M] */
    { 0x10de, 0x0a72 }, /* GT218 [GeForce 310M] */
    { 0x10de, 0x0a73 }, /* GT218 [GeForce 305M] */
    { 0x10de, 0x0a74 }, /* GT218 [GeForce G210M] */
    { 0x10de, 0x0a75 }, /* GT218 [GeForce 310M] */
    { 0x10de, 0x0a78 }, /* GT218GL [Quadro FX 380 LP] */
    { 0x10de, 0x0a7c }, /* GT218 [Quadro FX 380M] */
    { 0x10de, 0x0ca0 }, /* GT215 [GeForce GT 330] */
    { 0x10de, 0x0ca2 }, /* GT215 [GeForce GT 320] */
    { 0x10de, 0x0ca3 }, /* GT215 [GeForce GT 240] */
    { 0x10de, 0x0ca4 }, /* GT215 [GeForce GT 340] */
    { 0x10de, 0x0ca7 }, /* GT215 [GeForce GT 330] */
    { 0x10de, 0x0ca8 }, /* GT215 [GeForce GTS 260M] */
    { 0x10de, 0x0ca9 }, /* GT215 [GeForce GTS 250M] */
    { 0x10de, 0x0caf }, /* GT215 [GeForce GT 335M] */
    { 0x10de, 0x0cb0 }, /* GT215 [GeForce GTS 350M] */
    { 0x10de, 0x0cb1 }, /* GT215 [GeForce GTS 360M] */
    { 0x10de, 0x0cbc }, /* GT215 [Quadro FX 1800M] */
    { 0x10de, 0x0dc4 }, /* GF106 [GeForce 450 GTS] */
    { 0x10de, 0x0dcd }, /* GF108 [GeForce GT 555M] */
    { 0x10de, 0x0dd1 }, /* GF106 [GeForce GTX 460M] */
    { 0x10de, 0x0dd2 }, /* GF106 [GeForce GT 445M] */
    { 0x10de, 0x0dd8 }, /* GF106GL [Quadro 2000] */
    { 0x10de, 0x0dda }, /* GF106 [Quadro 2000M] */
    { 0x10de, 0x0de0 }, /* [GeForce GT 440] */
    { 0x10de, 0x0de1 }, /* GF108 [GeForce GT 430] */
    { 0x10de, 0x0de2 }, /* GeForce GT 420 */
    { 0x10de, 0x0dee }, /* GF108 [GeForce GT 415M] */
    { 0x10de, 0x0df0 }, /* GF108 [GeForce GT 425M] */
    { 0x10de, 0x0df4 }, /* GF106 [GeForce GT 555M] */
    { 0x10de, 0x0df8 }, /* GF108 [Quadro 600] */
    { 0x10de, 0x0dfa }, /* GF108 [Quadro 1000M] */
    { 0x10de, 0x0e22 }, /* GF104 [GeForce GTX 460] */
    { 0x10de, 0x0e3a }, /* GF104 [Quadro 3000M] */
    { 0x10de, 0x0e3b }, /* GF104 [Quadro 4000M] */
    { 0x10de, 0x1040 }, /* GT520 [GeForce GT520] */
    { 0x10de, 0x1056 }, /* GF108 [Quadro NVS 4200M] */
    { 0x10de, 0x1080 }, /* GF110 [GeForce GTX 580] */
    { 0x10de, 0x1081 }, /* GF110 [GeForce GTX 570] */
    { 0x10de, 0x1082 }, /* GF114 [GeForce GTX 560 Ti] */
    { 0x10de, 0x1086 }, /* GF110 [GeForce GTX 570 HD] */
    { 0x10de, 0x10c3 }, /* GT218 [GeForce 8400 GS] */
    { 0x10de, 0x10d8 }, /* GT218 [NVS 300] */
    { 0x10de, 0x1200 }, /* GF110 [GeForce GTX 560 Ti] */
    { 0x10de, 0x1201 }, /* GF114 [GeForce GTX 560] */
    { 0x10de, 0x1244 }, /* GF116 [GeForce GTX 550 Ti] */

    /* Entries missing in official list */
    { 0x10de, 0x0e23 }, /* GF104 [GeForce GTX 460 SE] */ 
    { 0x0000, 0x0000 }
};

#endif /* NOVUEAU_PCIIDS_H */

