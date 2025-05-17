int main() {
    const char *parent_txid = "abcd1234..."; // TXID you're spending from
    int vout = 0;                            // Output index
    const char *dest_address = "";          // Optional
    unsigned char payload[] = "hello-vobtc"; // Your signaling or voice data

    place_call(parent_txid, vout, dest_address, payload, sizeof(payload) - 1);
    return 0;
}
