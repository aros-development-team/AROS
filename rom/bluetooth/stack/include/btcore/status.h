#ifndef BTCORE_STATUS_H
#define BTCORE_STATUS_H

typedef enum bt_status
{
    BT_OK = 0,
    BT_ERR_INVALID_ARGUMENT,
    BT_ERR_BUFFER_OVERFLOW,   /* writer has no room left for the requested data */
    BT_ERR_BUFFER_UNDERFLOW,  /* reader has fewer bytes left than requested */
    BT_ERR_NO_RESOURCES,      /* a fixed-size pool (e.g. command queue slots) is full */
    BT_ERR_IO,                /* a platform transport/device operation failed */
    BT_ERR_INVALID_STATE,     /* operation does not fit the current state */
    BT_ERR_BUSY,              /* try again later (e.g. out of flow control credits) */
    BT_ERR_NOT_FOUND,
    BT_ERR_ALREADY            /* requested object already exists */
} bt_status_t;

#endif /* BTCORE_STATUS_H */
