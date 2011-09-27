/*

Copyright (C) 2011 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include <proto/exec.h>

#include "device.h"

#include "encryption_protos.h"

static VOID WEPEncrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base);
static BOOL WEPDecrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, struct DevBase *base);
static VOID TKIPEncrypt(struct DevUnit *unit, UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base);
static BOOL TKIPDecrypt(struct DevUnit *unit, UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base);
static VOID CCMPSetIV(struct DevUnit *unit, UBYTE *data, UBYTE *buffer,
   struct DevBase *base);
static VOID CCMPEncrypt(struct DevUnit *unit, const UBYTE *header,
   const UBYTE *data, UWORD size, UBYTE *buffer, struct DevBase *base);
static BOOL CCMPCheckIV(struct DevUnit *unit, UBYTE *data, UWORD *key_no,
   struct DevBase *base);
static BOOL CCMPDecrypt(struct DevUnit *unit, const UBYTE *header,
   const UBYTE *data, UWORD size, UBYTE *buffer, UWORD key_no,
   struct DevBase *base);
VOID UpdateMIC(ULONG *left, ULONG *right, const ULONG *data,
   ULONG count);
static VOID TKIPKeyMix1(UWORD *ttak, const UWORD *tk, const UWORD *ta,
   ULONG iv_high, struct DevBase *base);
VOID TKIPKeyMix2(UBYTE *rc4_seed, const UWORD *ttak, const UWORD *tk,
   UWORD iv16, struct DevBase *base);
VOID EOREncrypt(const ULONG *data, ULONG *buffer, ULONG *key,
   struct DevBase *base);
static VOID AESKeyMix(ULONG *stream, const ULONG *key,
   struct DevBase *base);
VOID AESEncrypt(const ULONG *data, ULONG *buffer, ULONG *key,
   struct DevBase *base);


static const UBYTE mic_padding[] =
   {0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


const UWORD sbox[] =
{
   0xC6A5, 0xF884, 0xEE99, 0xF68D, 0xFF0D, 0xD6BD, 0xDEB1, 0x9154,
   0x6050, 0x0203, 0xCEA9, 0x567D, 0xE719, 0xB562, 0x4DE6, 0xEC9A,
   0x8F45, 0x1F9D, 0x8940, 0xFA87, 0xEF15, 0xB2EB, 0x8EC9, 0xFB0B,
   0x41EC, 0xB367, 0x5FFD, 0x45EA, 0x23BF, 0x53F7, 0xE496, 0x9B5B,
   0x75C2, 0xE11C, 0x3DAE, 0x4C6A, 0x6C5A, 0x7E41, 0xF502, 0x834F,
   0x685C, 0x51F4, 0xD134, 0xF908, 0xE293, 0xAB73, 0x6253, 0x2A3F,
   0x080C, 0x9552, 0x4665, 0x9D5E, 0x3028, 0x37A1, 0x0A0F, 0x2FB5,
   0x0E09, 0x2436, 0x1B9B, 0xDF3D, 0xCD26, 0x4E69, 0x7FCD, 0xEA9F,
   0x121B, 0x1D9E, 0x5874, 0x342E, 0x362D, 0xDCB2, 0xB4EE, 0x5BFB,
   0xA4F6, 0x764D, 0xB761, 0x7DCE, 0x527B, 0xDD3E, 0x5E71, 0x1397,
   0xA6F5, 0xB968, 0x0000, 0xC12C, 0x4060, 0xE31F, 0x79C8, 0xB6ED,
   0xD4BE, 0x8D46, 0x67D9, 0x724B, 0x94DE, 0x98D4, 0xB0E8, 0x854A,
   0xBB6B, 0xC52A, 0x4FE5, 0xED16, 0x86C5, 0x9AD7, 0x6655, 0x1194,
   0x8ACF, 0xE910, 0x0406, 0xFE81, 0xA0F0, 0x7844, 0x25BA, 0x4BE3,
   0xA2F3, 0x5DFE, 0x80C0, 0x058A, 0x3FAD, 0x21BC, 0x7048, 0xF104,
   0x63DF, 0x77C1, 0xAF75, 0x4263, 0x2030, 0xE51A, 0xFD0E, 0xBF6D,
   0x814C, 0x1814, 0x2635, 0xC32F, 0xBEE1, 0x35A2, 0x88CC, 0x2E39,
   0x9357, 0x55F2, 0xFC82, 0x7A47, 0xC8AC, 0xBAE7, 0x322B, 0xE695,
   0xC0A0, 0x1998, 0x9ED1, 0xA37F, 0x4466, 0x547E, 0x3BAB, 0x0B83,
   0x8CCA, 0xC729, 0x6BD3, 0x283C, 0xA779, 0xBCE2, 0x161D, 0xAD76,
   0xDB3B, 0x6456, 0x744E, 0x141E, 0x92DB, 0x0C0A, 0x486C, 0xB8E4,
   0x9F5D, 0xBD6E, 0x43EF, 0xC4A6, 0x39A8, 0x31A4, 0xD337, 0xF28B,
   0xD532, 0x8B43, 0x6E59, 0xDAB7, 0x018C, 0xB164, 0x9CD2, 0x49E0,
   0xD8B4, 0xACFA, 0xF307, 0xCF25, 0xCAAF, 0xF48E, 0x47E9, 0x1018,
   0x6FD5, 0xF088, 0x4A6F, 0x5C72, 0x3824, 0x57F1, 0x73C7, 0x9751,
   0xCB23, 0xA17C, 0xE89C, 0x3E21, 0x96DD, 0x61DC, 0x0D86, 0x0F85,
   0xE090, 0x7C42, 0x71C4, 0xCCAA, 0x90D8, 0x0605, 0xF701, 0x1C12,
   0xC2A3, 0x6A5F, 0xAEF9, 0x69D0, 0x1791, 0x9958, 0x3A27, 0x27B9,
   0xD938, 0xEB13, 0x2BB3, 0x2233, 0xD2BB, 0xA970, 0x0789, 0x33A7,
   0x2DB6, 0x3C22, 0x1592, 0xC920, 0x8749, 0xAAFF, 0x5078, 0xA57A,
   0x038F, 0x59F8, 0x0980, 0x1A17, 0x65DA, 0xD731, 0x84C6, 0xD0B8,
   0x82C3, 0x29B0, 0x5A77, 0x1E11, 0x7BCB, 0xA8FC, 0x6DD6, 0x2C3A
};


const ULONG crc32[] =
{
   0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
   0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
   0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
   0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
   0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
   0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
   0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
   0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
   0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
   0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
   0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
   0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
   0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
   0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
   0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
   0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
   0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
   0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
   0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
   0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
   0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
   0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
   0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
   0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
   0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
   0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
   0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
   0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
   0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
   0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
   0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
   0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
   0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
   0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
   0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
   0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
   0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
   0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
   0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
   0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
   0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
   0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
   0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
   0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
   0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
   0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
   0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
   0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
   0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
   0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
   0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
   0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
   0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
   0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
   0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
   0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
   0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
   0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
   0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
   0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
   0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
   0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
   0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
   0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


#define SBox(A) \
   ({ \
      UWORD _SBox_A = (A); \
      UBYTE _SBox_low = _SBox_A; \
      UBYTE _SBox_high = _SBox_A >> 8; \
      _SBox_A = sbox[_SBox_low] ^ FlipWord(sbox[_SBox_high]); \
   })


const ULONG te0[] =
{
   0xc66363a5, 0xf87c7c84, 0xee777799, 0xf67b7b8d,
   0xfff2f20d, 0xd66b6bbd, 0xde6f6fb1, 0x91c5c554,
   0x60303050, 0x02010103, 0xce6767a9, 0x562b2b7d,
   0xe7fefe19, 0xb5d7d762, 0x4dababe6, 0xec76769a,
   0x8fcaca45, 0x1f82829d, 0x89c9c940, 0xfa7d7d87,
   0xeffafa15, 0xb25959eb, 0x8e4747c9, 0xfbf0f00b,
   0x41adadec, 0xb3d4d467, 0x5fa2a2fd, 0x45afafea,
   0x239c9cbf, 0x53a4a4f7, 0xe4727296, 0x9bc0c05b,
   0x75b7b7c2, 0xe1fdfd1c, 0x3d9393ae, 0x4c26266a,
   0x6c36365a, 0x7e3f3f41, 0xf5f7f702, 0x83cccc4f,
   0x6834345c, 0x51a5a5f4, 0xd1e5e534, 0xf9f1f108,
   0xe2717193, 0xabd8d873, 0x62313153, 0x2a15153f,
   0x0804040c, 0x95c7c752, 0x46232365, 0x9dc3c35e,
   0x30181828, 0x379696a1, 0x0a05050f, 0x2f9a9ab5,
   0x0e070709, 0x24121236, 0x1b80809b, 0xdfe2e23d,
   0xcdebeb26, 0x4e272769, 0x7fb2b2cd, 0xea75759f,
   0x1209091b, 0x1d83839e, 0x582c2c74, 0x341a1a2e,
   0x361b1b2d, 0xdc6e6eb2, 0xb45a5aee, 0x5ba0a0fb,
   0xa45252f6, 0x763b3b4d, 0xb7d6d661, 0x7db3b3ce,
   0x5229297b, 0xdde3e33e, 0x5e2f2f71, 0x13848497,
   0xa65353f5, 0xb9d1d168, 0x00000000, 0xc1eded2c,
   0x40202060, 0xe3fcfc1f, 0x79b1b1c8, 0xb65b5bed,
   0xd46a6abe, 0x8dcbcb46, 0x67bebed9, 0x7239394b,
   0x944a4ade, 0x984c4cd4, 0xb05858e8, 0x85cfcf4a,
   0xbbd0d06b, 0xc5efef2a, 0x4faaaae5, 0xedfbfb16,
   0x864343c5, 0x9a4d4dd7, 0x66333355, 0x11858594,
   0x8a4545cf, 0xe9f9f910, 0x04020206, 0xfe7f7f81,
   0xa05050f0, 0x783c3c44, 0x259f9fba, 0x4ba8a8e3,
   0xa25151f3, 0x5da3a3fe, 0x804040c0, 0x058f8f8a,
   0x3f9292ad, 0x219d9dbc, 0x70383848, 0xf1f5f504,
   0x63bcbcdf, 0x77b6b6c1, 0xafdada75, 0x42212163,
   0x20101030, 0xe5ffff1a, 0xfdf3f30e, 0xbfd2d26d,
   0x81cdcd4c, 0x180c0c14, 0x26131335, 0xc3ecec2f,
   0xbe5f5fe1, 0x359797a2, 0x884444cc, 0x2e171739,
   0x93c4c457, 0x55a7a7f2, 0xfc7e7e82, 0x7a3d3d47,
   0xc86464ac, 0xba5d5de7, 0x3219192b, 0xe6737395,
   0xc06060a0, 0x19818198, 0x9e4f4fd1, 0xa3dcdc7f,
   0x44222266, 0x542a2a7e, 0x3b9090ab, 0x0b888883,
   0x8c4646ca, 0xc7eeee29, 0x6bb8b8d3, 0x2814143c,
   0xa7dede79, 0xbc5e5ee2, 0x160b0b1d, 0xaddbdb76,
   0xdbe0e03b, 0x64323256, 0x743a3a4e, 0x140a0a1e,
   0x924949db, 0x0c06060a, 0x4824246c, 0xb85c5ce4,
   0x9fc2c25d, 0xbdd3d36e, 0x43acacef, 0xc46262a6,
   0x399191a8, 0x319595a4, 0xd3e4e437, 0xf279798b,
   0xd5e7e732, 0x8bc8c843, 0x6e373759, 0xda6d6db7,
   0x018d8d8c, 0xb1d5d564, 0x9c4e4ed2, 0x49a9a9e0,
   0xd86c6cb4, 0xac5656fa, 0xf3f4f407, 0xcfeaea25,
   0xca6565af, 0xf47a7a8e, 0x47aeaee9, 0x10080818,
   0x6fbabad5, 0xf0787888, 0x4a25256f, 0x5c2e2e72,
   0x381c1c24, 0x57a6a6f1, 0x73b4b4c7, 0x97c6c651,
   0xcbe8e823, 0xa1dddd7c, 0xe874749c, 0x3e1f1f21,
   0x964b4bdd, 0x61bdbddc, 0x0d8b8b86, 0x0f8a8a85,
   0xe0707090, 0x7c3e3e42, 0x71b5b5c4, 0xcc6666aa,
   0x904848d8, 0x06030305, 0xf7f6f601, 0x1c0e0e12,
   0xc26161a3, 0x6a35355f, 0xae5757f9, 0x69b9b9d0,
   0x17868691, 0x99c1c158, 0x3a1d1d27, 0x279e9eb9,
   0xd9e1e138, 0xebf8f813, 0x2b9898b3, 0x22111133,
   0xd26969bb, 0xa9d9d970, 0x078e8e89, 0x339494a7,
   0x2d9b9bb6, 0x3c1e1e22, 0x15878792, 0xc9e9e920,
   0x87cece49, 0xaa5555ff, 0x50282878, 0xa5dfdf7a,
   0x038c8c8f, 0x59a1a1f8, 0x09898980, 0x1a0d0d17,
   0x65bfbfda, 0xd7e6e631, 0x844242c6, 0xd06868b8,
   0x824141c3, 0x299999b0, 0x5a2d2d77, 0x1e0f0f11,
   0x7bb0b0cb, 0xa85454fc, 0x6dbbbbd6, 0x2c16163a
};


const ULONG te1[] =
{
   0xa5c66363, 0x84f87c7c, 0x99ee7777, 0x8df67b7b,
   0x0dfff2f2, 0xbdd66b6b, 0xb1de6f6f, 0x5491c5c5,
   0x50603030, 0x03020101, 0xa9ce6767, 0x7d562b2b,
   0x19e7fefe, 0x62b5d7d7, 0xe64dabab, 0x9aec7676,
   0x458fcaca, 0x9d1f8282, 0x4089c9c9, 0x87fa7d7d,
   0x15effafa, 0xebb25959, 0xc98e4747, 0x0bfbf0f0,
   0xec41adad, 0x67b3d4d4, 0xfd5fa2a2, 0xea45afaf,
   0xbf239c9c, 0xf753a4a4, 0x96e47272, 0x5b9bc0c0,
   0xc275b7b7, 0x1ce1fdfd, 0xae3d9393, 0x6a4c2626,
   0x5a6c3636, 0x417e3f3f, 0x02f5f7f7, 0x4f83cccc,
   0x5c683434, 0xf451a5a5, 0x34d1e5e5, 0x08f9f1f1,
   0x93e27171, 0x73abd8d8, 0x53623131, 0x3f2a1515,
   0x0c080404, 0x5295c7c7, 0x65462323, 0x5e9dc3c3,
   0x28301818, 0xa1379696, 0x0f0a0505, 0xb52f9a9a,
   0x090e0707, 0x36241212, 0x9b1b8080, 0x3ddfe2e2,
   0x26cdebeb, 0x694e2727, 0xcd7fb2b2, 0x9fea7575,
   0x1b120909, 0x9e1d8383, 0x74582c2c, 0x2e341a1a,
   0x2d361b1b, 0xb2dc6e6e, 0xeeb45a5a, 0xfb5ba0a0,
   0xf6a45252, 0x4d763b3b, 0x61b7d6d6, 0xce7db3b3,
   0x7b522929, 0x3edde3e3, 0x715e2f2f, 0x97138484,
   0xf5a65353, 0x68b9d1d1, 0x00000000, 0x2cc1eded,
   0x60402020, 0x1fe3fcfc, 0xc879b1b1, 0xedb65b5b,
   0xbed46a6a, 0x468dcbcb, 0xd967bebe, 0x4b723939,
   0xde944a4a, 0xd4984c4c, 0xe8b05858, 0x4a85cfcf,
   0x6bbbd0d0, 0x2ac5efef, 0xe54faaaa, 0x16edfbfb,
   0xc5864343, 0xd79a4d4d, 0x55663333, 0x94118585,
   0xcf8a4545, 0x10e9f9f9, 0x06040202, 0x81fe7f7f,
   0xf0a05050, 0x44783c3c, 0xba259f9f, 0xe34ba8a8,
   0xf3a25151, 0xfe5da3a3, 0xc0804040, 0x8a058f8f,
   0xad3f9292, 0xbc219d9d, 0x48703838, 0x04f1f5f5,
   0xdf63bcbc, 0xc177b6b6, 0x75afdada, 0x63422121,
   0x30201010, 0x1ae5ffff, 0x0efdf3f3, 0x6dbfd2d2,
   0x4c81cdcd, 0x14180c0c, 0x35261313, 0x2fc3ecec,
   0xe1be5f5f, 0xa2359797, 0xcc884444, 0x392e1717,
   0x5793c4c4, 0xf255a7a7, 0x82fc7e7e, 0x477a3d3d,
   0xacc86464, 0xe7ba5d5d, 0x2b321919, 0x95e67373,
   0xa0c06060, 0x98198181, 0xd19e4f4f, 0x7fa3dcdc,
   0x66442222, 0x7e542a2a, 0xab3b9090, 0x830b8888,
   0xca8c4646, 0x29c7eeee, 0xd36bb8b8, 0x3c281414,
   0x79a7dede, 0xe2bc5e5e, 0x1d160b0b, 0x76addbdb,
   0x3bdbe0e0, 0x56643232, 0x4e743a3a, 0x1e140a0a,
   0xdb924949, 0x0a0c0606, 0x6c482424, 0xe4b85c5c,
   0x5d9fc2c2, 0x6ebdd3d3, 0xef43acac, 0xa6c46262,
   0xa8399191, 0xa4319595, 0x37d3e4e4, 0x8bf27979,
   0x32d5e7e7, 0x438bc8c8, 0x596e3737, 0xb7da6d6d,
   0x8c018d8d, 0x64b1d5d5, 0xd29c4e4e, 0xe049a9a9,
   0xb4d86c6c, 0xfaac5656, 0x07f3f4f4, 0x25cfeaea,
   0xafca6565, 0x8ef47a7a, 0xe947aeae, 0x18100808,
   0xd56fbaba, 0x88f07878, 0x6f4a2525, 0x725c2e2e,
   0x24381c1c, 0xf157a6a6, 0xc773b4b4, 0x5197c6c6,
   0x23cbe8e8, 0x7ca1dddd, 0x9ce87474, 0x213e1f1f,
   0xdd964b4b, 0xdc61bdbd, 0x860d8b8b, 0x850f8a8a,
   0x90e07070, 0x427c3e3e, 0xc471b5b5, 0xaacc6666,
   0xd8904848, 0x05060303, 0x01f7f6f6, 0x121c0e0e,
   0xa3c26161, 0x5f6a3535, 0xf9ae5757, 0xd069b9b9,
   0x91178686, 0x5899c1c1, 0x273a1d1d, 0xb9279e9e,
   0x38d9e1e1, 0x13ebf8f8, 0xb32b9898, 0x33221111,
   0xbbd26969, 0x70a9d9d9, 0x89078e8e, 0xa7339494,
   0xb62d9b9b, 0x223c1e1e, 0x92158787, 0x20c9e9e9,
   0x4987cece, 0xffaa5555, 0x78502828, 0x7aa5dfdf,
   0x8f038c8c, 0xf859a1a1, 0x80098989, 0x171a0d0d,
   0xda65bfbf, 0x31d7e6e6, 0xc6844242, 0xb8d06868,
   0xc3824141, 0xb0299999, 0x775a2d2d, 0x111e0f0f,
   0xcb7bb0b0, 0xfca85454, 0xd66dbbbb, 0x3a2c1616
};


const ULONG te2[] =
{
   0x63a5c663, 0x7c84f87c, 0x7799ee77, 0x7b8df67b,
   0xf20dfff2, 0x6bbdd66b, 0x6fb1de6f, 0xc55491c5,
   0x30506030, 0x01030201, 0x67a9ce67, 0x2b7d562b,
   0xfe19e7fe, 0xd762b5d7, 0xabe64dab, 0x769aec76,
   0xca458fca, 0x829d1f82, 0xc94089c9, 0x7d87fa7d,
   0xfa15effa, 0x59ebb259, 0x47c98e47, 0xf00bfbf0,
   0xadec41ad, 0xd467b3d4, 0xa2fd5fa2, 0xafea45af,
   0x9cbf239c, 0xa4f753a4, 0x7296e472, 0xc05b9bc0,
   0xb7c275b7, 0xfd1ce1fd, 0x93ae3d93, 0x266a4c26,
   0x365a6c36, 0x3f417e3f, 0xf702f5f7, 0xcc4f83cc,
   0x345c6834, 0xa5f451a5, 0xe534d1e5, 0xf108f9f1,
   0x7193e271, 0xd873abd8, 0x31536231, 0x153f2a15,
   0x040c0804, 0xc75295c7, 0x23654623, 0xc35e9dc3,
   0x18283018, 0x96a13796, 0x050f0a05, 0x9ab52f9a,
   0x07090e07, 0x12362412, 0x809b1b80, 0xe23ddfe2,
   0xeb26cdeb, 0x27694e27, 0xb2cd7fb2, 0x759fea75,
   0x091b1209, 0x839e1d83, 0x2c74582c, 0x1a2e341a,
   0x1b2d361b, 0x6eb2dc6e, 0x5aeeb45a, 0xa0fb5ba0,
   0x52f6a452, 0x3b4d763b, 0xd661b7d6, 0xb3ce7db3,
   0x297b5229, 0xe33edde3, 0x2f715e2f, 0x84971384,
   0x53f5a653, 0xd168b9d1, 0x00000000, 0xed2cc1ed,
   0x20604020, 0xfc1fe3fc, 0xb1c879b1, 0x5bedb65b,
   0x6abed46a, 0xcb468dcb, 0xbed967be, 0x394b7239,
   0x4ade944a, 0x4cd4984c, 0x58e8b058, 0xcf4a85cf,
   0xd06bbbd0, 0xef2ac5ef, 0xaae54faa, 0xfb16edfb,
   0x43c58643, 0x4dd79a4d, 0x33556633, 0x85941185,
   0x45cf8a45, 0xf910e9f9, 0x02060402, 0x7f81fe7f,
   0x50f0a050, 0x3c44783c, 0x9fba259f, 0xa8e34ba8,
   0x51f3a251, 0xa3fe5da3, 0x40c08040, 0x8f8a058f,
   0x92ad3f92, 0x9dbc219d, 0x38487038, 0xf504f1f5,
   0xbcdf63bc, 0xb6c177b6, 0xda75afda, 0x21634221,
   0x10302010, 0xff1ae5ff, 0xf30efdf3, 0xd26dbfd2,
   0xcd4c81cd, 0x0c14180c, 0x13352613, 0xec2fc3ec,
   0x5fe1be5f, 0x97a23597, 0x44cc8844, 0x17392e17,
   0xc45793c4, 0xa7f255a7, 0x7e82fc7e, 0x3d477a3d,
   0x64acc864, 0x5de7ba5d, 0x192b3219, 0x7395e673,
   0x60a0c060, 0x81981981, 0x4fd19e4f, 0xdc7fa3dc,
   0x22664422, 0x2a7e542a, 0x90ab3b90, 0x88830b88,
   0x46ca8c46, 0xee29c7ee, 0xb8d36bb8, 0x143c2814,
   0xde79a7de, 0x5ee2bc5e, 0x0b1d160b, 0xdb76addb,
   0xe03bdbe0, 0x32566432, 0x3a4e743a, 0x0a1e140a,
   0x49db9249, 0x060a0c06, 0x246c4824, 0x5ce4b85c,
   0xc25d9fc2, 0xd36ebdd3, 0xacef43ac, 0x62a6c462,
   0x91a83991, 0x95a43195, 0xe437d3e4, 0x798bf279,
   0xe732d5e7, 0xc8438bc8, 0x37596e37, 0x6db7da6d,
   0x8d8c018d, 0xd564b1d5, 0x4ed29c4e, 0xa9e049a9,
   0x6cb4d86c, 0x56faac56, 0xf407f3f4, 0xea25cfea,
   0x65afca65, 0x7a8ef47a, 0xaee947ae, 0x08181008,
   0xbad56fba, 0x7888f078, 0x256f4a25, 0x2e725c2e,
   0x1c24381c, 0xa6f157a6, 0xb4c773b4, 0xc65197c6,
   0xe823cbe8, 0xdd7ca1dd, 0x749ce874, 0x1f213e1f,
   0x4bdd964b, 0xbddc61bd, 0x8b860d8b, 0x8a850f8a,
   0x7090e070, 0x3e427c3e, 0xb5c471b5, 0x66aacc66,
   0x48d89048, 0x03050603, 0xf601f7f6, 0x0e121c0e,
   0x61a3c261, 0x355f6a35, 0x57f9ae57, 0xb9d069b9,
   0x86911786, 0xc15899c1, 0x1d273a1d, 0x9eb9279e,
   0xe138d9e1, 0xf813ebf8, 0x98b32b98, 0x11332211,
   0x69bbd269, 0xd970a9d9, 0x8e89078e, 0x94a73394,
   0x9bb62d9b, 0x1e223c1e, 0x87921587, 0xe920c9e9,
   0xce4987ce, 0x55ffaa55, 0x28785028, 0xdf7aa5df,
   0x8c8f038c, 0xa1f859a1, 0x89800989, 0x0d171a0d,
   0xbfda65bf, 0xe631d7e6, 0x42c68442, 0x68b8d068,
   0x41c38241, 0x99b02999, 0x2d775a2d, 0x0f111e0f,
   0xb0cb7bb0, 0x54fca854, 0xbbd66dbb, 0x163a2c16
};


const ULONG te3[] =
{
   0x6363a5c6, 0x7c7c84f8, 0x777799ee, 0x7b7b8df6,
   0xf2f20dff, 0x6b6bbdd6, 0x6f6fb1de, 0xc5c55491,
   0x30305060, 0x01010302, 0x6767a9ce, 0x2b2b7d56,
   0xfefe19e7, 0xd7d762b5, 0xababe64d, 0x76769aec,
   0xcaca458f, 0x82829d1f, 0xc9c94089, 0x7d7d87fa,
   0xfafa15ef, 0x5959ebb2, 0x4747c98e, 0xf0f00bfb,
   0xadadec41, 0xd4d467b3, 0xa2a2fd5f, 0xafafea45,
   0x9c9cbf23, 0xa4a4f753, 0x727296e4, 0xc0c05b9b,
   0xb7b7c275, 0xfdfd1ce1, 0x9393ae3d, 0x26266a4c,
   0x36365a6c, 0x3f3f417e, 0xf7f702f5, 0xcccc4f83,
   0x34345c68, 0xa5a5f451, 0xe5e534d1, 0xf1f108f9,
   0x717193e2, 0xd8d873ab, 0x31315362, 0x15153f2a,
   0x04040c08, 0xc7c75295, 0x23236546, 0xc3c35e9d,
   0x18182830, 0x9696a137, 0x05050f0a, 0x9a9ab52f,
   0x0707090e, 0x12123624, 0x80809b1b, 0xe2e23ddf,
   0xebeb26cd, 0x2727694e, 0xb2b2cd7f, 0x75759fea,
   0x09091b12, 0x83839e1d, 0x2c2c7458, 0x1a1a2e34,
   0x1b1b2d36, 0x6e6eb2dc, 0x5a5aeeb4, 0xa0a0fb5b,
   0x5252f6a4, 0x3b3b4d76, 0xd6d661b7, 0xb3b3ce7d,
   0x29297b52, 0xe3e33edd, 0x2f2f715e, 0x84849713,
   0x5353f5a6, 0xd1d168b9, 0x00000000, 0xeded2cc1,
   0x20206040, 0xfcfc1fe3, 0xb1b1c879, 0x5b5bedb6,
   0x6a6abed4, 0xcbcb468d, 0xbebed967, 0x39394b72,
   0x4a4ade94, 0x4c4cd498, 0x5858e8b0, 0xcfcf4a85,
   0xd0d06bbb, 0xefef2ac5, 0xaaaae54f, 0xfbfb16ed,
   0x4343c586, 0x4d4dd79a, 0x33335566, 0x85859411,
   0x4545cf8a, 0xf9f910e9, 0x02020604, 0x7f7f81fe,
   0x5050f0a0, 0x3c3c4478, 0x9f9fba25, 0xa8a8e34b,
   0x5151f3a2, 0xa3a3fe5d, 0x4040c080, 0x8f8f8a05,
   0x9292ad3f, 0x9d9dbc21, 0x38384870, 0xf5f504f1,
   0xbcbcdf63, 0xb6b6c177, 0xdada75af, 0x21216342,
   0x10103020, 0xffff1ae5, 0xf3f30efd, 0xd2d26dbf,
   0xcdcd4c81, 0x0c0c1418, 0x13133526, 0xecec2fc3,
   0x5f5fe1be, 0x9797a235, 0x4444cc88, 0x1717392e,
   0xc4c45793, 0xa7a7f255, 0x7e7e82fc, 0x3d3d477a,
   0x6464acc8, 0x5d5de7ba, 0x19192b32, 0x737395e6,
   0x6060a0c0, 0x81819819, 0x4f4fd19e, 0xdcdc7fa3,
   0x22226644, 0x2a2a7e54, 0x9090ab3b, 0x8888830b,
   0x4646ca8c, 0xeeee29c7, 0xb8b8d36b, 0x14143c28,
   0xdede79a7, 0x5e5ee2bc, 0x0b0b1d16, 0xdbdb76ad,
   0xe0e03bdb, 0x32325664, 0x3a3a4e74, 0x0a0a1e14,
   0x4949db92, 0x06060a0c, 0x24246c48, 0x5c5ce4b8,
   0xc2c25d9f, 0xd3d36ebd, 0xacacef43, 0x6262a6c4,
   0x9191a839, 0x9595a431, 0xe4e437d3, 0x79798bf2,
   0xe7e732d5, 0xc8c8438b, 0x3737596e, 0x6d6db7da,
   0x8d8d8c01, 0xd5d564b1, 0x4e4ed29c, 0xa9a9e049,
   0x6c6cb4d8, 0x5656faac, 0xf4f407f3, 0xeaea25cf,
   0x6565afca, 0x7a7a8ef4, 0xaeaee947, 0x08081810,
   0xbabad56f, 0x787888f0, 0x25256f4a, 0x2e2e725c,
   0x1c1c2438, 0xa6a6f157, 0xb4b4c773, 0xc6c65197,
   0xe8e823cb, 0xdddd7ca1, 0x74749ce8, 0x1f1f213e,
   0x4b4bdd96, 0xbdbddc61, 0x8b8b860d, 0x8a8a850f,
   0x707090e0, 0x3e3e427c, 0xb5b5c471, 0x6666aacc,
   0x4848d890, 0x03030506, 0xf6f601f7, 0x0e0e121c,
   0x6161a3c2, 0x35355f6a, 0x5757f9ae, 0xb9b9d069,
   0x86869117, 0xc1c15899, 0x1d1d273a, 0x9e9eb927,
   0xe1e138d9, 0xf8f813eb, 0x9898b32b, 0x11113322,
   0x6969bbd2, 0xd9d970a9, 0x8e8e8907, 0x9494a733,
   0x9b9bb62d, 0x1e1e223c, 0x87879215, 0xe9e920c9,
   0xcece4987, 0x5555ffaa, 0x28287850, 0xdfdf7aa5,
   0x8c8c8f03, 0xa1a1f859, 0x89898009, 0x0d0d171a,
   0xbfbfda65, 0xe6e631d7, 0x4242c684, 0x6868b8d0,
   0x4141c382, 0x9999b029, 0x2d2d775a, 0x0f0f111e,
   0xb0b0cb7b, 0x5454fca8, 0xbbbbd66d, 0x16163a2c
};


const ULONG te4[] =
{
   0x63636363, 0x7c7c7c7c, 0x77777777, 0x7b7b7b7b,
   0xf2f2f2f2, 0x6b6b6b6b, 0x6f6f6f6f, 0xc5c5c5c5,
   0x30303030, 0x01010101, 0x67676767, 0x2b2b2b2b,
   0xfefefefe, 0xd7d7d7d7, 0xabababab, 0x76767676,
   0xcacacaca, 0x82828282, 0xc9c9c9c9, 0x7d7d7d7d,
   0xfafafafa, 0x59595959, 0x47474747, 0xf0f0f0f0,
   0xadadadad, 0xd4d4d4d4, 0xa2a2a2a2, 0xafafafaf,
   0x9c9c9c9c, 0xa4a4a4a4, 0x72727272, 0xc0c0c0c0,
   0xb7b7b7b7, 0xfdfdfdfd, 0x93939393, 0x26262626,
   0x36363636, 0x3f3f3f3f, 0xf7f7f7f7, 0xcccccccc,
   0x34343434, 0xa5a5a5a5, 0xe5e5e5e5, 0xf1f1f1f1,
   0x71717171, 0xd8d8d8d8, 0x31313131, 0x15151515,
   0x04040404, 0xc7c7c7c7, 0x23232323, 0xc3c3c3c3,
   0x18181818, 0x96969696, 0x05050505, 0x9a9a9a9a,
   0x07070707, 0x12121212, 0x80808080, 0xe2e2e2e2,
   0xebebebeb, 0x27272727, 0xb2b2b2b2, 0x75757575,
   0x09090909, 0x83838383, 0x2c2c2c2c, 0x1a1a1a1a,
   0x1b1b1b1b, 0x6e6e6e6e, 0x5a5a5a5a, 0xa0a0a0a0,
   0x52525252, 0x3b3b3b3b, 0xd6d6d6d6, 0xb3b3b3b3,
   0x29292929, 0xe3e3e3e3, 0x2f2f2f2f, 0x84848484,
   0x53535353, 0xd1d1d1d1, 0x00000000, 0xedededed,
   0x20202020, 0xfcfcfcfc, 0xb1b1b1b1, 0x5b5b5b5b,
   0x6a6a6a6a, 0xcbcbcbcb, 0xbebebebe, 0x39393939,
   0x4a4a4a4a, 0x4c4c4c4c, 0x58585858, 0xcfcfcfcf,
   0xd0d0d0d0, 0xefefefef, 0xaaaaaaaa, 0xfbfbfbfb,
   0x43434343, 0x4d4d4d4d, 0x33333333, 0x85858585,
   0x45454545, 0xf9f9f9f9, 0x02020202, 0x7f7f7f7f,
   0x50505050, 0x3c3c3c3c, 0x9f9f9f9f, 0xa8a8a8a8,
   0x51515151, 0xa3a3a3a3, 0x40404040, 0x8f8f8f8f,
   0x92929292, 0x9d9d9d9d, 0x38383838, 0xf5f5f5f5,
   0xbcbcbcbc, 0xb6b6b6b6, 0xdadadada, 0x21212121,
   0x10101010, 0xffffffff, 0xf3f3f3f3, 0xd2d2d2d2,
   0xcdcdcdcd, 0x0c0c0c0c, 0x13131313, 0xecececec,
   0x5f5f5f5f, 0x97979797, 0x44444444, 0x17171717,
   0xc4c4c4c4, 0xa7a7a7a7, 0x7e7e7e7e, 0x3d3d3d3d,
   0x64646464, 0x5d5d5d5d, 0x19191919, 0x73737373,
   0x60606060, 0x81818181, 0x4f4f4f4f, 0xdcdcdcdc,
   0x22222222, 0x2a2a2a2a, 0x90909090, 0x88888888,
   0x46464646, 0xeeeeeeee, 0xb8b8b8b8, 0x14141414,
   0xdededede, 0x5e5e5e5e, 0x0b0b0b0b, 0xdbdbdbdb,
   0xe0e0e0e0, 0x32323232, 0x3a3a3a3a, 0x0a0a0a0a,
   0x49494949, 0x06060606, 0x24242424, 0x5c5c5c5c,
   0xc2c2c2c2, 0xd3d3d3d3, 0xacacacac, 0x62626262,
   0x91919191, 0x95959595, 0xe4e4e4e4, 0x79797979,
   0xe7e7e7e7, 0xc8c8c8c8, 0x37373737, 0x6d6d6d6d,
   0x8d8d8d8d, 0xd5d5d5d5, 0x4e4e4e4e, 0xa9a9a9a9,
   0x6c6c6c6c, 0x56565656, 0xf4f4f4f4, 0xeaeaeaea,
   0x65656565, 0x7a7a7a7a, 0xaeaeaeae, 0x08080808,
   0xbabababa, 0x78787878, 0x25252525, 0x2e2e2e2e,
   0x1c1c1c1c, 0xa6a6a6a6, 0xb4b4b4b4, 0xc6c6c6c6,
   0xe8e8e8e8, 0xdddddddd, 0x74747474, 0x1f1f1f1f,
   0x4b4b4b4b, 0xbdbdbdbd, 0x8b8b8b8b, 0x8a8a8a8a,
   0x70707070, 0x3e3e3e3e, 0xb5b5b5b5, 0x66666666,
   0x48484848, 0x03030303, 0xf6f6f6f6, 0x0e0e0e0e,
   0x61616161, 0x35353535, 0x57575757, 0xb9b9b9b9,
   0x86868686, 0xc1c1c1c1, 0x1d1d1d1d, 0x9e9e9e9e,
   0xe1e1e1e1, 0xf8f8f8f8, 0x98989898, 0x11111111,
   0x69696969, 0xd9d9d9d9, 0x8e8e8e8e, 0x94949494,
   0x9b9b9b9b, 0x1e1e1e1e, 0x87878787, 0xe9e9e9e9,
   0xcececece, 0x55555555, 0x28282828, 0xdfdfdfdf,
   0x8c8c8c8c, 0xa1a1a1a1, 0x89898989, 0x0d0d0d0d,
   0xbfbfbfbf, 0xe6e6e6e6, 0x42424242, 0x68686868,
   0x41414141, 0x99999999, 0x2d2d2d2d, 0x0f0f0f0f,
   0xb0b0b0b0, 0x54545454, 0xbbbbbbbb, 0x16161616
};


static const ULONG rcon[] =
{
   0x01000000, 0x02000000, 0x04000000, 0x08000000,
   0x10000000, 0x20000000, 0x40000000, 0x80000000,
   0x1B000000, 0x36000000
};



/****i* prism2.device/WriteClearFragment ***********************************
*
*   NAME
*	WriteClearFragment -- Install unencrypted fragment data.
*
*   SYNOPSIS
*	WriteClearFragment(unit, header, data, size,
*	    buffer)
*
*	VOID WriteClearFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data address may be equal to the output buffer address,
*	but they may not overlap in any other way.
*
****************************************************************************
*
*/

VOID WriteClearFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   if(buffer != data)
      CopyMem(data, buffer, *size);

   return;
}



/****i* prism2.device/EncryptWEPFragment ***********************************
*
*   NAME
*	EncryptWEPFragment -- Encrypt a fragment using the WEP cipher.
*
*   SYNOPSIS
*	EncryptWEPFragment(unit, header, data, size,
*	    buffer)
*
*	VOID EncryptWEPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least 8 bytes bigger than the input
*	data, so that the IV and ICV can be added.
*
****************************************************************************
*
*/

VOID EncryptWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   WEPEncrypt(unit, data, *size, buffer, FALSE, base);
   *size += IV_SIZE + ICV_SIZE;

   return;
}



/****i* prism2.device/WriteWEPFragment *************************************
*
*   NAME
*	WriteWEPFragment -- Prepare a fragment to use the WEP cipher.
*
*   SYNOPSIS
*	WriteWEPFragment(unit, header, data, size,
*	    buffer)
*
*	VOID WriteWEPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data address may be four bytes greater than the output
*	buffer address, but they may not overlap in any other way.
*
****************************************************************************
*
*/

VOID WriteWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   WEPEncrypt(unit, data, *size, buffer, TRUE, base);
   if(buffer + IV_SIZE != data)
      CopyMem(data, buffer + IV_SIZE, *size);
   *size += IV_SIZE + ICV_SIZE;

   return;
}



/****i* prism2.device/EncryptTKIPFragment **********************************
*
*   NAME
*	EncryptTKIPFragment -- Encrypt a fragment using the TKIP cipher.
*
*   SYNOPSIS
*	EncryptTKIPFragment(unit, header, data,
*	    size, buffer)
*
*	VOID EncryptTKIPFragment(struct DevUnit *, UBYTE *, UBYTE *,
*	    UWORD *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least 12 bytes bigger than the input
*	data, so that the Extended IV and the ICV can be added.
*
****************************************************************************
*
*/

VOID EncryptTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   TKIPEncrypt(unit, data, *size, buffer, FALSE, base);
   *size += EIV_SIZE + ICV_SIZE;

   return;
}



/****i* prism2.device/WriteTKIPFragment ************************************
*
*   NAME
*	WriteTKIPFragment -- Prepare a fragment to use the TKIP cipher.
*
*   SYNOPSIS
*	WriteTKIPFragment(unit, header, data, size,
*	    buffer)
*
*	VOID WriteTKIPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data address may be eight bytes greater than the output
*	buffer address, but they may not overlap in any other way.
*
****************************************************************************
*
*/

VOID WriteTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   TKIPEncrypt(unit, data, *size, buffer, TRUE, base);
   if(buffer + EIV_SIZE != data)
      CopyMem(data, buffer + EIV_SIZE, *size);
   *size += EIV_SIZE + ICV_SIZE;

   return;
}



/****i* prism2.device/EncryptCCMPFragment **********************************
*
*   NAME
*	EncryptCCMPFragment -- Encrypt a fragment using the CCMP cipher.
*
*   SYNOPSIS
*	EncryptCCMPFragment(unit, header, data,
*	    size, buffer)
*
*	VOID EncryptCCMPFragment(struct DevUnit *, UBYTE *, UBYTE *,
*	    UWORD *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least 16 bytes bigger than the input
*	data, so that the Extended IV and the MIC can be added.
*
****************************************************************************
*
*/

VOID EncryptCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   CCMPSetIV(unit, data, buffer, base);
   CCMPEncrypt(unit, header, data, *size, buffer + EIV_SIZE, base);
   *size += EIV_SIZE + MIC_SIZE;

   return;
}



/****i* prism2.device/WriteCCMPFragment ************************************
*
*   NAME
*	WriteCCMPFragment -- Prepare a fragment to use the CCMP cipher.
*
*   SYNOPSIS
*	WriteCCMPFragment(unit, header, data, size,
*	    buffer)
*
*	VOID WriteCCMPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data address may be eight bytes greater than the output
*	buffer address, but they may not overlap in any other way.
*
****************************************************************************
*
*/

VOID WriteCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   CCMPSetIV(unit, data, buffer, base);
   if(buffer + EIV_SIZE != data)
      CopyMem(data, buffer + EIV_SIZE, *size);
   *size += EIV_SIZE + MIC_SIZE;

   return;
}



/****i* prism2.device/ReadClearFragment *************************************
*
*   NAME
*	ReadClearFragment -- Extract data from an unencrypted fragment.
*
*   SYNOPSIS
*	success = ReadClearFragment(unit, header, data, size,
*	    buffer)
*
*	BOOL ReadClearFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
****************************************************************************
*
*/

BOOL ReadClearFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   CopyMem(data, buffer, *size);

   return TRUE;
}



/****i* prism2.device/DecryptWEPFragment ***********************************
*
*   NAME
*	DecryptWEPFragment -- Decrypt a fragment using the WEP cipher.
*
*   SYNOPSIS
*	success = DecryptWEPFragment(unit, header, data, size,
*	    buffer)
*
*	BOOL DecryptWEPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

BOOL DecryptWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   BOOL success;

   success = WEPDecrypt(unit, data, *size, buffer, base);
   *size -= IV_SIZE + ICV_SIZE;

   return success;
}



/****i* prism2.device/ReadWEPFragment **************************************
*
*   NAME
*	ReadWEPFragment -- Extract data from a WEP fragment.
*
*   SYNOPSIS
*	success = ReadWEPFragment(unit, header, data, size,
*	    buffer)
*
*	BOOL ReadWEPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
****************************************************************************
*
*/

BOOL ReadWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   *size -= IV_SIZE + ICV_SIZE;
   CopyMem(data + IV_SIZE, buffer, *size);

   return TRUE;
}



/****i* prism2.device/DecryptTKIPFragment **********************************
*
*   NAME
*	DecryptTKIPFragment -- Decrypt a fragment using the TKIP cipher.
*
*   SYNOPSIS
*	success = DecryptTKIPFragment(unit, header, data,
*	    size, buffer)
*
*	BOOL DecryptTKIPFragment(struct DevUnit *, UBYTE *, UBYTE *,
*	    UWORD *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

BOOL DecryptTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   BOOL success;

   success = TKIPDecrypt(unit, data, *size, buffer, FALSE,
      base);
   *size -= EIV_SIZE + ICV_SIZE;

   return success;
}



/****i* prism2.device/ReadTKIPFragment *************************************
*
*   NAME
*	ReadTKIPFragment -- Extract data from a TKIP fragment.
*
*   SYNOPSIS
*	success = ReadTKIPFragment(unit, header, data, size,
*	    buffer)
*
*	BOOL ReadTKIPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
****************************************************************************
*
*/

BOOL ReadTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   BOOL success;

   success = TKIPDecrypt(unit, data, *size, buffer, TRUE,
      base);
   *size -= EIV_SIZE + ICV_SIZE;
   if(success)
      CopyMem(data + EIV_SIZE, buffer, *size);

   return success;
}



/****i* prism2.device/DecryptCCMPFragment **********************************
*
*   NAME
*	DecryptCCMPFragment -- Decrypt a fragment using the CCMP cipher.
*
*   SYNOPSIS
*	success = DecryptCCMPFragment(unit, header, data,
*	    size, buffer)
*
*	BOOL DecryptCCMPFragment(struct DevUnit *, UBYTE *, UBYTE *,
*	    UWORD *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

BOOL DecryptCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   BOOL success = FALSE;
   UWORD key_no;
   struct CCMPKey *key;

   if(CCMPCheckIV(unit, data, &key_no, base))
   {
      *size -= EIV_SIZE;
      success = CCMPDecrypt(unit, header, data + EIV_SIZE, *size, buffer,
         key_no, base);
      *size -= MIC_SIZE;
   }

   /* Increment sequence counter */

   if(success)
   {
      key = &unit->keys[key_no].u.ccmp;
      if(++key->rx_iv_low == 0)
         key->rx_iv_high++;
   }

   return success;
}



/****i* prism2.device/ReadCCMPFragment *************************************
*
*   NAME
*	ReadCCMPFragment -- Extract data from a CCMP fragment.
*
*   SYNOPSIS
*	success = ReadCCMPFragment(unit, header, data, size,
*	    buffer)
*
*	BOOL ReadCCMPFragment(struct DevUnit *, UBYTE *, UBYTE *, UWORD *,
*	    UBYTE *);
*
****************************************************************************
*
*/

BOOL ReadCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base)
{
   BOOL success;
   UWORD key_no;
   struct CCMPKey *key;

   success = CCMPCheckIV(unit, data, &key_no, base);
   *size -= EIV_SIZE + MIC_SIZE;
   if(success)
      CopyMem(data + EIV_SIZE, buffer, *size);

   /* Increment sequence counter */

   if(success)
   {
      key = &unit->keys[key_no].u.ccmp;
      if(++key->rx_iv_low == 0)
         key->rx_iv_high++;
   }

   return success;
}



/****i* prism2.device/TKIPEncryptFrame *************************************
*
*   NAME
*	TKIPEncryptFrame -- Append the Michael MIC to a frame.
*
*   SYNOPSIS
*	TKIPEncryptFrame(unit, header, data, size,
*	    buffer)
*
*	VOID TKIPEncryptFrame(struct DevUnit *, UBYTE *, UBYTE *, UWORD,
*	    UBYTE *);
*
*   NOTES
*	The input data address may be equal to the output buffer address,
*	but they may not overlap in any other way.
*
*	The output buffer must be at least 8 bytes bigger than the input
*	data, so that the MIC can be added.
*
****************************************************************************
*
*/

VOID TKIPEncryptFrame(struct DevUnit *unit, const UBYTE *header,
   UBYTE *data, UWORD size, UBYTE *buffer, struct DevBase *base)
{
   ULONG *mic_key, mic[2];
   ULONG mic_l, mic_r, zero = 0;
   struct TKIPKey *key = &unit->keys[unit->tx_key_no].u.tkip;

   /* Calculate MIC and append to packet data */

   if(data != buffer)
      CopyMem(data, buffer, size);

   mic_key = key->tx_mic_key;
   mic_l = LELong(mic_key[0]);
   mic_r = LELong(mic_key[1]);
   UpdateMIC(&mic_l, &mic_r, (ULONG *)header, 3);
   UpdateMIC(&mic_l, &mic_r, &zero, 1);
   CopyMem(mic_padding, buffer + size, sizeof(mic_padding));
   UpdateMIC(&mic_l, &mic_r, (ULONG *)buffer, size / 4 + 2);
   mic[0] = MakeLELong(mic_l);
   mic[1] = MakeLELong(mic_r);
   CopyMem(mic, buffer + size, sizeof(mic));

   return;
}



/****i* prism2.device/TKIPDecryptFrame *************************************
*
*   NAME
*	TKIPDecryptFrame -- Check that a frame's MIC is correct.
*
*   SYNOPSIS
*	success = TKIPDecryptFrame(unit, header, data, size,
*	    buffer, key_no)
*
*	BOOL TKIPDecryptFrame(struct DevUnit *, UBYTE *, UBYTE *, UWORD,
*	    UBYTE *, UWORD);
*
*   NOTES
*	The input data address may be equal to the output buffer address,
*	but they may not overlap in any other way.
*
****************************************************************************
*
*/

BOOL TKIPDecryptFrame(struct DevUnit *unit, const UBYTE *header,
   UBYTE *data, UWORD size, UBYTE *buffer, UWORD key_no,
   struct DevBase *base)
{
   ULONG *mic_key, mic[2];
   ULONG mic_l, mic_r, zero = 0;

   /* Calculate MIC and compare it with MIC received */

   size -= sizeof(mic);
   CopyMem(buffer + size, mic, sizeof(mic));
   CopyMem(mic_padding, buffer + size, sizeof(mic_padding));

   mic_key = unit->keys[key_no].u.tkip.rx_mic_key;
   mic_l = LELong(mic_key[0]);
   mic_r = LELong(mic_key[1]);
   UpdateMIC(&mic_l, &mic_r, (ULONG *)header, 3);
   UpdateMIC(&mic_l, &mic_r, &zero, 1);
   UpdateMIC(&mic_l, &mic_r, (ULONG *)buffer, size / sizeof(ULONG) + 2);

   if(data != buffer)
      CopyMem(data, buffer, size);

   return LELong(mic[0]) == mic_l && LELong(mic[1]) == mic_r;
}



/****i* prism2.device/WEPEncrypt *******************************************
*
*   NAME
*	WEPEncrypt -- Encrypt a fragment using the WEP cipher.
*
*   SYNOPSIS
*	WEPEncrypt(unit, data, size, buffer, seed,
*	    iv_only)
*
*	VOID WEPEncrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, UBYTE *,
*	    BOOL);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is at least 4 bytes lower than the input address.
*
****************************************************************************
*
*/

static VOID WEPEncrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base)
{
   UWORD i;
   ULONG iv;
   const UBYTE *p;
   UBYTE seed[16], *q;
   struct WEPKey *key = &unit->keys[unit->tx_key_no].u.wep;

   /* Create RC4 seed from IV and WEP key */

   iv = MakeBELong(++key->tx_iv);
   for(i = 0, p = (UBYTE *)&iv + 1, q = seed; i < 3; i++)
      *q++ = *p++;
   if(!iv_only)
      for(i = 0, p = key->key; i < key->length; i++)
         *q++ = *p++;

   /* Preface encrypted data with the IV and key ID */

   for(i = 0; i < 3; i++)
      buffer[i] = seed[i];
   buffer[3] = unit->tx_key_no << 6;

   /* Encrypt data and CRC */

   if(!iv_only)
      RC4Encrypt(unit, data, size, buffer + 4, seed, base);

   return;
}



/****i* prism2.device/WEPDecrypt *******************************************
*
*   NAME
*	WEPDecrypt -- Decrypt a fragment using the WEP cipher.
*
*   SYNOPSIS
*	WEPDecrypt(unit, data, size, buffer)
*
*	VOID WEPDecrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

static BOOL WEPDecrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, struct DevBase *base)
{
   UWORD i;
   const UBYTE *p;
   UBYTE seed[16], *q;
   const struct WEPKey *key = &unit->keys[data[3] >> 6].u.wep;

   /* Create RC4 seed from IV and WEP key */

   for(i = 0, p = data, q = seed; i < 3; i++)
      *q++ = *p++;
   for(i = 0, p = key->key; i < key->length; i++)
      *q++ = *p++;

   /* Decrypt data */

   return RC4Decrypt(unit, data + 4, size - 4, buffer, seed, base);
}



/****i* prism2.device/TKIPEncrypt ******************************************
*
*   NAME
*	TKIPEncrypt -- Encrypt a fragment using the TKIP cipher.
*
*   SYNOPSIS
*	TKIPEncrypt(unit, data, size, buffer, iv_only)
*
*	VOID TKIPEncrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, BOOL);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least 12 bytes bigger than the input
*	data, so that the Extended IV and the ICV can be added.
*
****************************************************************************
*
*/

static VOID TKIPEncrypt(struct DevUnit *unit, UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base)
{
   UBYTE seed[16];
   struct TKIPKey *key = &unit->keys[unit->tx_key_no].u.tkip;

   /* Create RC4 seed from IV, TA and temporal key */

   if(!iv_only)
   {
      if(!key->tx_ttak_set)
      {
         TKIPKeyMix1(key->tx_ttak, (UWORD *)key->key,
            (UWORD *)unit->address, key->tx_iv_high, base);
         key->tx_ttak_set = TRUE;
      }
      TKIPKeyMix2(seed, key->tx_ttak, (UWORD *)key->key, key->tx_iv_low,
         base);
   }

   /* Preface encrypted data with IV, key ID, TKIP flag and extended IV */

   buffer[0] = key->tx_iv_low >> 8;
   buffer[1] = (key->tx_iv_low >> 8 | 0x20) & 0x7f;
   buffer[2] = key->tx_iv_low;
   buffer[3] = unit->tx_key_no << 6 | 0x20;
   buffer += 4;
   *(ULONG *)buffer =
      MakeLELong(key->tx_iv_high);
   buffer += 4;

   /* Encrypt data and CRC */

   if(!iv_only)
      RC4Encrypt(unit, data, size, buffer, seed, base);

   /* Increment 48-bit sequence counter */

   if((++key->tx_iv_low) == 0)
   {
      key->tx_iv_high++;
      key->tx_ttak_set = FALSE;
   }

   return;
}



/****i* prism2.device/TKIPDecrypt ******************************************
*
*   NAME
*	TKIPDecrypt -- Decrypt a fragment using the TKIP cipher.
*
*   SYNOPSIS
*	success = TKIPDecrypt(unit, data, size, buffer, iv_only)
*
*	BOOL TKIPDecrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, BOOL);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

static BOOL TKIPDecrypt(struct DevUnit *unit, UBYTE *data, UWORD size,
   UBYTE *buffer, BOOL iv_only, struct DevBase *base)
{
   UBYTE seed[16];
   ULONG iv_low, iv_high;
   BOOL success = TRUE;
   UWORD key_no;
   struct TKIPKey *key;

   /* Extract IV, key ID, TKIP flag and extended IV */

   iv_low = data[0] << 8 | data[2];
   key_no = data[3] >> 6;
   data += 4;
   iv_high = LELong(*(ULONG *)data);
   data += 4;

   /* Check for replay attack */

   key = &unit->keys[key_no].u.tkip;
   if(iv_high < key->rx_iv_high
      || iv_high == key->rx_iv_high && iv_low < key->rx_iv_low)
      success = FALSE;

   /* TO DO: replay timeout */

   if(success && !iv_only)
   {
      /* Create RC4 seed from IV, TA and temporal key */

      if(iv_high > key->rx_iv_high)
         key->rx_ttak_set = FALSE;
      if(!key->rx_ttak_set)
      {
         TKIPKeyMix1(key->rx_ttak,
            (UWORD *)unit->keys[key_no].u.tkip.key,
            (UWORD *)unit->bssid, iv_high, base);
         key->rx_ttak_set = TRUE;
      }
      TKIPKeyMix2(seed, key->rx_ttak,
         (UWORD *)unit->keys[key_no].u.tkip.key, iv_low, base);

      /* Decrypt data and check ICV/CRC */

      size -= EIV_SIZE;
      success = RC4Decrypt(unit, data, size, buffer, seed, base);
   }

   /* Increment 48-bit sequence counter */

   if(success)
   {
      key->rx_iv_high = iv_high;
      key->rx_iv_low = iv_low;
      if(++key->rx_iv_low == 0)
      {
         key->rx_iv_high++;
         key->rx_ttak_set = FALSE;
      }
   }

   return success;
}



/****i* prism2.device/CCMPSetIV ********************************************
*
*   NAME
*	CCMPSetIV -- Add CCMP IV to a fragment.
*
*   SYNOPSIS
*	CCMPSetIV(unit, data, buffer)
*
*	VOID CCMPSetIV(struct DevUnit *, UBYTE *, UBYTE *);
*
****************************************************************************
*
*/

static VOID CCMPSetIV(struct DevUnit *unit, UBYTE *data, UBYTE *buffer,
   struct DevBase *base)
{
   struct CCMPKey *key = &unit->keys[unit->tx_key_no].u.ccmp;

   /* Increment 48-bit sequence counter */

   if((++key->tx_iv_low) == 0)
   {
      key->tx_iv_high++;
   }

   /* Preface encrypted data with IV, key ID, CCMP flag and extended IV */

   buffer[0] = key->tx_iv_low;
   buffer[1] = key->tx_iv_low >> 8;
   buffer[2] = 0;
   buffer[3] = unit->tx_key_no << 6 | 0x20;
   *(ULONG *)(buffer + 4) = MakeLELong(key->tx_iv_high);

   return;
}



/****i* prism2.device/CCMPEncrypt ******************************************
*
*   NAME
*	CCMPEncrypt -- Encrypt a fragment using the CCMP cipher.
*
*   SYNOPSIS
*	CCMPEncrypt(unit, header, data, size, buffer,
*	    iv_only)
*
*	VOID CCMPEncrypt(struct DevUnit *, UBYTE *, UBYTE *, UWORD, UBYTE *,
*	    BOOL);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least 16 bytes bigger than the input
*	data, so that the MIC can be added.
*
*   INTERNALS
*	For efficiency, the nonce and AAD are not constructed as separate
*	entities, but immediately integrated into their corresponding CBC
*	blocks.
*
****************************************************************************
*
*/

static VOID CCMPEncrypt(struct DevUnit *unit, const UBYTE *header,
   const UBYTE *data, UWORD size, UBYTE *buffer, struct DevBase *base)
{
   struct CCMPKey *key = &unit->keys[unit->tx_key_no].u.ccmp;
   UBYTE aad_blocks[16 * 2], nonce_block[16],
      *q;
   const UBYTE *p;
   UWORD i, j, block_count;
   ULONG n, data_block[4], mic_block[4], key_block[4];

   if(!key->stream_set)
   {
      AESKeyMix(key->stream, (ULONG *)key->key, base);
      key->stream_set = TRUE;
   }

   /* Construct AAD blocks */

   aad_blocks[0] = 0;
   aad_blocks[1] = 22;

   *(UWORD *)(aad_blocks + 2) = *(UWORD *)(header + WIFI_FRM_CONTROL)
      & MakeLEWord(WIFI_FRM_CONTROLF_VERSION | WIFI_FRM_CONTROLF_TYPE
      | (1 << 7) | WIFI_FRM_CONTROLF_TODS | WIFI_FRM_CONTROLF_FROMDS
      | WIFI_FRM_CONTROLF_MOREFRAGS | WIFI_FRM_CONTROLF_ORDER)
      | MakeLEWord(WIFI_FRM_CONTROLF_WEP);
   CopyMem(header + WIFI_FRM_ADDRESS1, aad_blocks + 4, ETH_ADDRESSSIZE * 3);
   *(UWORD *)(aad_blocks + 22) =
      *(UWORD *)(header + WIFI_FRM_SEQCONTROL) & MakeLEWord(0xf);

   *(ULONG *)(aad_blocks + 24) = 0;
   *(ULONG *)(aad_blocks + 28) = 0;

   /* Construct nonce block */

   nonce_block[0] = 0x59;

   nonce_block[1] = 0;
   CopyMem(header + WIFI_FRM_ADDRESS2, nonce_block + 2, ETH_ADDRESSSIZE);

   n = MakeBELong(key->tx_iv_high);
   for(i = 0, p = (UBYTE *)&n; i < 4; i++)
      nonce_block[8 + i] = p[i];
   n = MakeBELong(key->tx_iv_low);
   for(i = 0, p = (UBYTE *)&n + 2; i < 2; i++)
      nonce_block[12 + i] = p[i];

   *(UWORD *)(nonce_block + 14) = MakeBEWord(size);

   /* Initialise MIC block from nonce block */

   AESEncrypt((ULONG *)nonce_block, mic_block, key->stream, base);

   /* Integrate AAD into MIC */

   for(i = 0, p = aad_blocks; i < 2; i++, p += 16)
   {
      EOREncrypt((ULONG *)p, mic_block, mic_block, base);
      AESEncrypt(mic_block, mic_block, key->stream,
         base);
   }

   /* Encrypt data and calculate MIC */

   nonce_block[0] = 0x01;
   block_count = (size - 1) / 16 + 1;

   for(i = 1, j = size, p = data, q = buffer; i <= block_count; i++,
      p += 16, q += 16, j -= 16)
   {
      /* Update MIC */

      if(j < 16)
      {
         data_block[0] = 0;
         data_block[1] = 0;
         data_block[2] = 0;
         data_block[3] = 0;
         CopyMem(p, data_block, j);
      }
      else
         CopyMem(p, data_block, 16);
      EOREncrypt(data_block, mic_block, mic_block, base);
      AESEncrypt(mic_block, mic_block, key->stream, base);

      /* Encrypt next block */

      *(UWORD *)(nonce_block + 14) = MakeBEWord(i);
      AESEncrypt((ULONG *)nonce_block, key_block, key->stream, base);
      EOREncrypt((ULONG *)p, (ULONG *)q, key_block, base);
   }

   /* Append MIC */

   *(UWORD *)(nonce_block + 14) = 0;
   AESEncrypt((ULONG *)nonce_block, key_block, key->stream, base);
   EOREncrypt(mic_block, mic_block, key_block, base);
   CopyMem(mic_block, buffer + size, MIC_SIZE);

   return;
}



/****i* prism2.device/CCMPCheckIV ******************************************
*
*   NAME
*	CCMPCheckIV -- Process a CCMP fragment's IV.
*
*   SYNOPSIS
*	success = CCMPCheckIV(unit, data, key_no)
*
*	BOOL CCMPCheckIV(struct DevUnit *, UBYTE *, UWORD);
*
****************************************************************************
*
*/

static BOOL CCMPCheckIV(struct DevUnit *unit, UBYTE *data, UWORD *key_no,
   struct DevBase *base)
{
   BOOL success = TRUE;
   ULONG iv_low, iv_high;
   struct CCMPKey *key;

   /* Extract sequence counter and key ID */

   iv_low = data[1] << 8 | data[0];
   *key_no = data[3] >> 6;
   data += 4;
   iv_high = LELong(*(ULONG *)data);

   /* Check for replay attack */

   key = &unit->keys[*key_no].u.ccmp;
   if(iv_high < key->rx_iv_high
      || iv_high == key->rx_iv_high && iv_low < key->rx_iv_low)
      success = FALSE;

   /* TO DO: replay timeout */

   /* Store new sequence counter */

   if(success)
   {
      key->rx_iv_high = iv_high;
      key->rx_iv_low = iv_low;
   }

   return success;
}



/****i* prism2.device/CCMPDecrypt ******************************************
*
*   NAME
*	CCMPDecrypt -- Decrypt a fragment using the CCMP cipher.
*
*   SYNOPSIS
*	success = CCMPDecrypt(unit, header, data, size, buffer,
*	    iv_only)
*
*	BOOL CCMPDecrypt(struct DevUnit *, UBYTE *, UBYTE *, UWORD, UBYTE *,
*	    BOOL);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

static BOOL CCMPDecrypt(struct DevUnit *unit, const UBYTE *header,
   const UBYTE *data, UWORD size, UBYTE *buffer, UWORD key_no,
   struct DevBase *base)
{
   struct CCMPKey *key;
   UBYTE aad_blocks[16 * 2], nonce_block[16], *q;
   const UBYTE *p;
   UWORD i, j, block_count;
   ULONG n, mic_block[4], key_block[4];
   BOOL success = TRUE;
   ULONG data_block[4];

   /* Get key */

   size -= MIC_SIZE;
   key = &unit->keys[key_no].u.ccmp;
   if(!key->stream_set)
   {
      AESKeyMix(key->stream, (ULONG *)key->key, base);
      key->stream_set = TRUE;
   }

   /* Construct AAD blocks */

   aad_blocks[0] = 0;
   aad_blocks[1] = 22;

   *(UWORD *)(aad_blocks + 2) = *(UWORD *)(header + WIFI_FRM_CONTROL)
      & MakeLEWord(WIFI_FRM_CONTROLF_VERSION | WIFI_FRM_CONTROLF_TYPE
      | (1 << 7) | WIFI_FRM_CONTROLF_TODS | WIFI_FRM_CONTROLF_FROMDS
      | WIFI_FRM_CONTROLF_MOREFRAGS | WIFI_FRM_CONTROLF_ORDER)
      | MakeLEWord(WIFI_FRM_CONTROLF_WEP);
   CopyMem(header + WIFI_FRM_ADDRESS1, aad_blocks + 4, ETH_ADDRESSSIZE * 3);
   *(UWORD *)(aad_blocks + 22) =
      *(UWORD *)(header + WIFI_FRM_SEQCONTROL) & MakeLEWord(0xf);

   *(ULONG *)(aad_blocks + 24) = 0;
   *(ULONG *)(aad_blocks + 28) = 0;

   /* Construct nonce block */

   nonce_block[0] = 0x59;

   nonce_block[1] = 0;
   CopyMem(header + WIFI_FRM_ADDRESS2, nonce_block + 2, ETH_ADDRESSSIZE);

   n = MakeBELong(key->rx_iv_high);
   for(i = 0, p = (UBYTE *)&n; i < 4; i++)
      nonce_block[8 + i] = p[i];
   n = MakeBELong(key->rx_iv_low);
   for(i = 0, p = (UBYTE *)&n + 2; i < 2; i++)
      nonce_block[12 + i] = p[i];

   *(UWORD *)(nonce_block + 14) = MakeBEWord(size);

   /* Initialise MIC block from nonce block */

   AESEncrypt((ULONG *)nonce_block, mic_block, key->stream, base);

   /* Integrate AAD into MIC */

   for(i = 0, p = aad_blocks; i < 2; i++, p += 16)
   {
      EOREncrypt((ULONG *)p, mic_block, mic_block, base);
      AESEncrypt(mic_block, mic_block, key->stream, base);
   }

   /* Decrypt data and calculate MIC */

   nonce_block[0] = 0x01;
   block_count = (size - 1) / 16 + 1;

   for(i = 1, j = size, p = data, q = buffer; i <= block_count; i++,
      p += 16, q += 16, j -= 16)
   {
      /* Decrypt next block */

      *(UWORD *)(nonce_block + 14) = MakeBEWord(i);
      AESEncrypt((ULONG *)nonce_block, key_block, key->stream, base);
      EOREncrypt((ULONG *)p, (ULONG *)q, key_block, base);

      /* Update MIC */

      if(j < 16)
      {
         data_block[0] = 0;
         data_block[1] = 0;
         data_block[2] = 0;
         data_block[3] = 0;
         CopyMem(q, data_block, j);
      }
      else
      {
         CopyMem(q, data_block, 16);
      }
      EOREncrypt(data_block, mic_block, mic_block, base);
      AESEncrypt(mic_block, mic_block, key->stream, base);
   }

   /* Extract and check MIC */

   *(UWORD *)(nonce_block + 14) = 0;
   AESEncrypt((ULONG *)nonce_block, key_block, key->stream, base);
   EOREncrypt(mic_block, mic_block, key_block, base);
   CopyMem(data + size, mic_block + MIC_SIZE / sizeof(ULONG), MIC_SIZE);
   if(mic_block[0] != mic_block[2] || mic_block[1] != mic_block[3])
      success = FALSE;

   return success;
}



/****i* prism2.device/UpdateMIC ********************************************
*
*   NAME
*	UpdateMIC -- Include new data into a Michael MIC value.
*
*   SYNOPSIS
*	UpdateMIC(left, right, data, count)
*
*	VOID UpdateMIC(ULONG *, ULONG *, ULONG *, ULONG);
*
****************************************************************************
*
*/

VOID UpdateMIC(ULONG *left, ULONG *right, const ULONG *data,
   ULONG count)
{
   ULONG l = *left, r = *right, i;

   for(i = 0; i < count; i++)
   {
      l ^= LELong(*data++);
      r ^= l << 17 | l >> (32 - 17);
      l += r;
      r ^= ((l & 0x00ff00ff) << 8) | ((l & 0xff00ff00) >> 8);
      l += r;
      r ^= l << 3 | l >> (32 - 3);
      l += r;
      r ^= l >> 2 | l << (32 - 2);
      l += r;
   }

   *left = l;
   *right = r;

   return;
}



/****i* prism2.device/TKIPKeyMix1 ******************************************
*
*   NAME
*	TKIPKeyMix1 -- Apply phase 1 of the TKIP key-mixing function.
*
*   SYNOPSIS
*	TKIPKeyMix1(ttak, tk, ta, iv_high)
*
*	VOID TKIPKeyMix1(UWORD *, UWORD *, UWORD *, ULONG);
*
****************************************************************************
*
*/

static VOID TKIPKeyMix1(UWORD *ttak, const UWORD *tk, const UWORD *ta,
   ULONG iv_high, struct DevBase *base)
{
   const UWORD *p;
   UWORD i, *q, *r, a;

   ttak[0] = iv_high;
   ttak[1] = iv_high >> 16;
   ttak[2] = LEWord(ta[0]);
   ttak[3] = LEWord(ta[1]);
   ttak[4] = LEWord(ta[2]);

   for(i = 0; i < 8; i++)
   {
      p = tk + (i & 1);
      q = ttak;
      r = ttak + 4;
      a = *p++;
      *q++ += SBox(*r ^ a);
      r = ttak;
      *q++ += SBox(*r++ ^ *++p);
      p += 2;
      *q++ += SBox(*r++ ^ *p++);
      *q++ += SBox(*r++ ^ *++p);
      *q += SBox(*r++ ^ a) + i;
   }

   return;
}



/****i* prism2.device/TKIPKeyMix2 ******************************************
*
*   NAME
*	TKIPKeyMix2 -- Apply phase 2 of the TKIP key-mixing function.
*
*   SYNOPSIS
*	TKIPKeyMix2(rc4_seed, ttak, tk, iv16)
*
*	VOID TKIPKeyMix2(UBYTE *, UWORD *, UWORD *, UWORD);
*
****************************************************************************
*
*/

VOID TKIPKeyMix2(UBYTE *rc4_seed, const UWORD *ttak, const UWORD *tk,
   UWORD iv16, struct DevBase *base)
{
   const UWORD *p = ttak;
   UWORD i, temp_key[6], *q = temp_key, *r, a;

   for(i = 0; i < 5; i++)
      *q++ = *p++;
   a = *q++ = ttak[4] + iv16;

   p = tk;
   r = q = temp_key;
   *q++ += SBox(a ^ *p++);
   *q++ += SBox(*r++ ^ *p++);
   *q++ += SBox(*r++ ^ *p++);
   *q++ += SBox(*r++ ^ *p++);
   *q++ += SBox(*r++ ^ *p++);
   *q++ += SBox(*r++ ^ *p++);

   q = temp_key;
   a = *r++ ^ *p++;
   *q++ += a << 15 | a >> 1;
   r = temp_key;
   a = *r++ ^ *p++;
   *q++ += a << 15 | a >> 1;
   a = *r++;
   *q++ += a << 15 | a >> 1;
   a = *r++;
   *q++ += a << 15 | a >> 1;
   a = *r++;
   *q++ += a << 15 | a >> 1;
   a = *r;
   *q += a << 15 | a >> 1;

   *rc4_seed++ = iv16 >> 8;
   *rc4_seed++ = (iv16 >> 8 | 0x20) & 0x7f;
   *rc4_seed++ = iv16;
   *rc4_seed++ = (*q ^ tk[0]) >> 1;
   q = temp_key;
   for(i = 0; i < 6; i++)
   {
      a = *q++;
      *rc4_seed++ = a;
      *rc4_seed++ = a >> 8;
   }

   return;
}



/****i* prism2.device/RC4Encrypt *******************************************
*
*   NAME
*	RC4Encrypt -- Encrypt data using the RC4 cipher, and append CRC.
*
*   SYNOPSIS
*	RC4Encrypt(unit, data, size, buffer, seed)
*
*	VOID RC4Encrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
****************************************************************************
*
*/

VOID RC4Encrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, UBYTE *seed, struct DevBase *base)
{
   UWORD i, j, k;
   ULONG crc = ~0;
   const UBYTE *p;
   UBYTE s[256], *q, a, b, c;

   /* Initialise RC4 state */

   for(i = 0; i < 256; i++)
      s[i] = i;
   for(j = i = 0; i < 256; i++)
   {
      a = s[i];
      j = j + seed[i & 0xf] + a & 0xff;
      s[i] = s[j];
      s[j] = a;
   }

   /* Encrypt data and CRC */

   for(p = data, q = buffer, k = j = i = 0; k < size + 4; k++)
   {
      if(k == size)
      {
         *(ULONG *)q = MakeLELong(~crc);
         p = q;
      }
      i = i + 1 & 0xff;
      a = s[i];
      j = j + a & 0xff;
      s[i] = b = s[j];
      s[j] = a;
      c = *p++;
      *q++ = c ^ s[a + b & 0xff];
      crc = crc32[(crc ^ c) & 0xff] ^ crc >> 8;
   }

   return;
}


/****i* prism2.device/RC4Decrypt *******************************************
*
*   NAME
*	RC4Decrypt -- Decrypt data using the RC4 cipher, and check CRC.
*
*   SYNOPSIS
*	success = RC4Encrypt(unit, data, size, buffer, seed)
*
*	BOOL RC4Decrypt(struct DevUnit *, UBYTE *, UWORD, UBYTE *, UBYTE *);
*
*   NOTES
*	The input data may overlap the output buffer as long as the output
*	address is not higher than the input address.
*
*	The output buffer must be at least as big as the input data.
*
****************************************************************************
*
*/

BOOL RC4Decrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, UBYTE *seed, struct DevBase *base)
{
   UWORD i, j, k;
   ULONG crc = ~0;
   const UBYTE *p;
   UBYTE s[256], *q, a, b, c;

   /* Initialise RC4 state */

   for(i = 0; i < 256; i++)
      s[i] = i;
   for(j = i = 0; i < 256; i++)
   {
      a = s[i];
      j = j + seed[i & 0xf] + s[i] & 0xff;
      s[i] = s[j];
      s[j] = a;
   }

   /* Decrypt data and CRC */

   for(p = data, q = buffer, k = j = i = 0; k < size; k++)
   {
      i = i + 1 & 0xff;
      a = s[i];
      j = j + a & 0xff;
      b = s[j];
      s[j] = a;
      s[i] = b;
      c = *p++ ^ s[a + b & 0xff];
      if(k < size - 4)
         crc = crc32[(crc ^ c) & 0xff] ^ crc >> 8;
      *q++ = c;
   }

   /* Check CRC */

   return ~crc == LELong(*(ULONG *)(q - 4));
}


/****i* prism2.device/EOREncrypt *******************************************
*
*   NAME
*	EOREncrypt -- Apply exclusive-or to a block.
*
*   SYNOPSIS
*	EOREncrypt(data, buffer, key)
*
*	VOID EOREncrypt(UBYTE *, UBYTE *, UBYTE *);
*
*   NOTES
*#	The input data may overlap the output buffer as long as the output
*#	address is not higher than the input address.
*
****************************************************************************
*
*/

VOID EOREncrypt(const ULONG *data, ULONG *buffer, ULONG *key,
   struct DevBase *base)
{
    *buffer++ = *data++ ^ *key++;
    *buffer++ = *data++ ^ *key++;
    *buffer++ = *data++ ^ *key++;
    *buffer++ = *data++ ^ *key++;
}



/****i* prism2.device/AESKeyMix ********************************************
*
*   NAME
*	AESKeyMix -- Expand an AES key into a key stream.
*
*   SYNOPSIS
*	AESKeyMix(stream, key)
*
*	VOID AESKeyMix(struct DevUnit *, UBYTE *);
*
****************************************************************************
*
*/

static VOID AESKeyMix(ULONG *stream, const ULONG *key, struct DevBase *base)
{
   UWORD i;
   ULONG n;

   stream[0] = BELong(key[0]);
   stream[1] = BELong(key[1]);
   stream[2] = BELong(key[2]);
   stream[3] = BELong(key[3]);

   for(i = 0; i < 10; i++)
   {
      n = stream[3];
      stream[4] = stream[0] ^ (te4[(n >> 16) & 0xff] & 0xff000000)
         ^ (te4[(n >> 8) & 0xff] & 0x00ff0000)
         ^ (te4[n & 0xff] & 0x0000ff00)
         ^ (te4[n >> 24] & 0x000000ff) ^ rcon[i];
      stream[5] = stream[1] ^ stream[4];
      stream[6] = stream[2] ^ stream[5];
      stream[7] = stream[3] ^ stream[6];
      stream += 4;
   }
}



/****i* prism2.device/AESEncrypt *******************************************
*
*   NAME
*	AESEncrypt -- Encrypt a block using the AES cipher.
*
*   SYNOPSIS
*	AESEncrypt(data, buffer, key)
*
*	VOID AESEncrypt(UBYTE *, UBYTE *, UBYTE *);
*
****************************************************************************
*
*/

VOID AESEncrypt(const ULONG *data, ULONG *buffer, ULONG *key,
   struct DevBase *base)
{
   UWORD i = 5;
   ULONG s0, s1, s2, s3, t0, t1, t2, t3;


   s0 = BELong(data[0]) ^ key[0];
   s1 = BELong(data[1]) ^ key[1];
   s2 = BELong(data[2]) ^ key[2];
   s3 = BELong(data[3]) ^ key[3];

   while(TRUE)
   {
      t0 = te0[s0 >> 24] ^ te1[(s1 >> 16) & 0xff] ^ te2[(s2 >> 8) & 0xff]
         ^ te3[s3 & 0xff] ^ key[4];
      t1 = te0[s1 >> 24] ^ te1[(s2 >> 16) & 0xff] ^ te2[(s3 >> 8) & 0xff]
         ^ te3[s0 & 0xff] ^ key[5];
      t2 = te0[s2 >> 24] ^ te1[(s3 >> 16) & 0xff] ^ te2[(s0 >> 8) & 0xff]
         ^ te3[s1 & 0xff] ^ key[6];
      t3 = te0[s3 >> 24] ^ te1[(s0 >> 16) & 0xff] ^ te2[(s1 >> 8) & 0xff]
         ^ te3[s2 & 0xff] ^ key[7];

      key += 8;
      if(--i == 0)
         break;

      s0 = te0[t0 >> 24] ^ te1[(t1 >> 16) & 0xff] ^ te2[(t2 >> 8) & 0xff]
         ^ te3[t3 & 0xff] ^ key[0];
      s1 = te0[t1 >> 24] ^ te1[(t2 >> 16) & 0xff] ^ te2[(t3 >> 8) & 0xff]
         ^ te3[t0 & 0xff] ^ key[1];
      s2 = te0[t2 >> 24] ^ te1[(t3 >> 16) & 0xff] ^ te2[(t0 >> 8) & 0xff]
         ^ te3[t1 & 0xff] ^ key[2];
      s3 = te0[t3 >> 24] ^ te1[(t0 >> 16) & 0xff] ^ te2[(t1 >> 8) & 0xff]
         ^ te3[t2 & 0xff] ^ key[3];
   }

   s0 = (te4[t0 >> 24] & 0xff000000) ^ (te4[(t1 >> 16) & 0xff] & 0x00ff0000)
      ^ (te4[(t2 >> 8) & 0xff] & 0x0000ff00) ^ (te4[t3 & 0xff] & 0x000000ff)
      ^ key[0];
   buffer[0] = MakeBELong(s0);
   s1 = (te4[t1 >> 24] & 0xff000000) ^ (te4[(t2 >> 16) & 0xff] & 0x00ff0000)
      ^ (te4[(t3 >> 8) & 0xff] & 0x0000ff00) ^ (te4[t0 & 0xff] & 0x000000ff)
      ^ key[1];
   buffer[1] = MakeBELong(s1);
   s2 = (te4[t2 >> 24] & 0xff000000) ^ (te4[(t3 >> 16) & 0xff] & 0x00ff0000)
      ^ (te4[(t0 >> 8) & 0xff] & 0x0000ff00) ^ (te4[t1 & 0xff] & 0x000000ff)
      ^ key[2];
   buffer[2] = MakeBELong(s2);
   s3 = (te4[t3 >> 24] & 0xff000000) ^ (te4[(t0 >> 16) & 0xff] & 0x00ff0000)
      ^ (te4[(t1 >> 8) & 0xff] & 0x0000ff00) ^ (te4[t2 & 0xff] & 0x000000ff)
      ^ key[3];
   buffer[3] = MakeBELong(s3);

   return;
}



