#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

// Helper to reverse txid byte order
void reverse_hex(char *dst, const char *src) {
    for (int i = 0; i < 64; i += 2)
        sscanf(&src[62 - i], "%2c", &dst[i]);
    dst[64] = '\0';
}

// Function to check if rawtx spends from a specific txid
int tx_spends_parent(const unsigned char *rawtx, size_t len, const char *parent_txid) {
    const unsigned char *p = rawtx;
    if (len < 4) return 0;

    // Skip version (4 bytes)
    p += 4;

    // Skip optional segwit marker/flag if present
    int segwit = (p[0] == 0x00 && p[1] == 0x01);
    if (segwit) p += 2;

    // Read varint for input count
    uint64_t vin_count = *p;
    p += 1;

    for (uint64_t i = 0; i < vin_count; i++) {
        if (p + 36 > rawtx + len) return 0;

        char input_txid[65] = {0};
        for (int j = 0; j < 32; j++)
            sprintf(&input_txid[j * 2], "%02x", p[31 - j]);

        if (strncmp(input_txid, parent_txid, 64) == 0)
            return 1;

        p += 36; // Skip txid (32) + vout (4)

        // Skip scriptSig length and scriptSig itself
        uint8_t script_len = *p++;
        p += script_len;

        p += 4; // Skip sequence
    }

    return 0;
}

// Main function to subscribe and detect child txs
void monitor_child_tx(const char *zmq_address, const char *parent_txid) {
    void *context = zmq_ctx_new();
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, zmq_address);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "rawtx", 5);

    printf("Listening for child transactions of %s...\n", parent_txid);

    while (1) {
        zmq_msg_t topic_msg;
        zmq_msg_t tx_msg;

        zmq_msg_init(&topic_msg);
        zmq_msg_recv(&topic_msg, subscriber, 0);
        zmq_msg_close(&topic_msg);

        zmq_msg_init(&tx_msg);
        zmq_msg_recv(&tx_msg, subscriber, 0);

        const unsigned char *data = zmq_msg_data(&tx_msg);
        size_t len = zmq_msg_size(&tx_msg);

        if (tx_spends_parent(data, len, parent_txid)) {
            printf("Child transaction detected spending from %s!\n", parent_txid);
            // TODO: pass tx to Asterisk or other system here
        }

        zmq_msg_close(&tx_msg);

        usleep(100000); // 100ms
    }

    zmq_close(subscriber);
    zmq_ctx_destroy(context);
}
