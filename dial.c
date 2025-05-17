#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hex-encode byte array into a string
void hex_encode(const unsigned char *in, size_t len, char *out) {
    const char *hex = "0123456789abcdef";
    for (size_t i = 0; i < len; ++i) {
        out[i*2] = hex[in[i] >> 4];
        out[i*2 + 1] = hex[in[i] & 0x0F];
    }
    out[len*2] = '\0';
}

// Place "call" by broadcasting tx with OP_RETURN or small payment
void place_call(const char *parent_txid, int vout_index, const char *to_address, const unsigned char *data, size_t data_len) {
    char hex_data[2 * data_len + 1];
    hex_encode(data, data_len, hex_data);

    char cmd[4096];

    // Step 1: Create raw transaction with OP_RETURN
    snprintf(cmd, sizeof(cmd),
        "bitcoin-cli createrawtransaction "
        "[{\"txid\":\"%s\",\"vout\":%d}] "
        "{\"data\":\"%s\"}",
        parent_txid, vout_index, hex_data);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        perror("popen create");
        return;
    }

    char rawtx[4096];
    if (!fgets(rawtx, sizeof(rawtx), fp)) {
        perror("read rawtx");
        pclose(fp);
        return;
    }
    pclose(fp);
    rawtx[strcspn(rawtx, "\n")] = 0;

    // Step 2: Fund the transaction (optional: use fundrawtransaction if not manually choosing utxo)
    char fundedtx[4096];
    snprintf(cmd, sizeof(cmd), "bitcoin-cli fundrawtransaction \"%s\"", rawtx);
    fp = popen(cmd, "r");
    if (!fp) {
        perror("popen fund");
        return;
    }

    char json[4096];
    if (!fgets(json, sizeof(json), fp)) {
        perror("read fund");
        pclose(fp);
        return;
    }
    pclose(fp);

    // Extract hex string manually (could use JSON parser if desired)
    char *hex_ptr = strstr(json, "\"hex\":\"");
    if (!hex_ptr) {
        fprintf(stderr, "Failed to parse funded transaction\n");
        return;
    }

    hex_ptr += strlen("\"hex\":\"");
    char *end = strchr(hex_ptr, '"');
    if (end) *end = 0;

    // Step 3: Sign the transaction
    snprintf(cmd, sizeof(cmd), "bitcoin-cli signrawtransactionwithwallet \"%s\"", hex_ptr);
    fp = popen(cmd, "r");
    if (!fp) {
        perror("popen sign");
        return;
    }

    if (!fgets(json, sizeof(json), fp)) {
        perror("read sign");
        pclose(fp);
        return;
    }
    pclose(fp);

    hex_ptr = strstr(json, "\"hex\":\"");
    if (!hex_ptr) {
        fprintf(stderr, "Failed to parse signed transaction\n");
        return;
    }

    hex_ptr += strlen("\"hex\":\"");
    end = strchr(hex_ptr, '"');
    if (end) *end = 0;

    // Step 4: Broadcast the transaction
    snprintf(cmd, sizeof(cmd), "bitcoin-cli sendrawtransaction \"%s\"", hex_ptr);
    printf("Broadcasting: %s\n", cmd);
    system(cmd);
}
