/* private.rs - Various private chunks
 * 2024 (c) a dinosaur
 * SPDX-License-Identifier: Zlib */

use super::private_chunk;


// https://wiki.amigaos.net/wiki/IFF_FORM_and_Chunk_Registry
private_chunk!(PhotonPrivate, b"BHSM");         // "private Photon Paint chunk"
private_chunk!(PhotonPrivateScreens, b"BHCP");  // "private Photon Paint chunk (screens)"
//privateChunk!(PhotonPrivateBrushes, b"BHBA");
