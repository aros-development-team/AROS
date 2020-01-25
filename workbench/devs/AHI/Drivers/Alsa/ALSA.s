
    FORM_START  AHIM

    CHUNK_START AUDN
    .asciz      "alsa"
    CHUNK_END

    CHUNK_START AUDM
1:
    LONG2       AHIDB_AudioID,     0x00420002
    LONG2       AHIDB_Volume,      TRUE
    LONG2       AHIDB_Panning,     TRUE
    LONG2       AHIDB_Stereo,      TRUE
    LONG2       AHIDB_HiFi,        FALSE
    LONG2       AHIDB_MultTable,   FALSE
    LONG2       AHIDB_Name,        2f-1b
    LONG        TAG_DONE
2:
    .asciz      "ALSA:16 bit stereo++"
    CHUNK_END


    FORM_END

    .END
