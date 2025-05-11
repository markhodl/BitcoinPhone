✅ Requirements
  Bitcoin Core configured with: zmqpubrawtx=tcp://127.0.0.1:28332
  Link with libzmq: -lzmq
  Link with libbitcoinconsensus if needed (for validation, not used here)

✅ Function Overview
  Input: const char *parent_txid
  Subscribes to rawtx topic
  Every 100ms:
    Receives new TX
    Parses the inputs
    If any input references parent_txid, logs it as a "child"

✅ Build Instructions
  gcc -o monitor_tx monitor_tx.c -lzmq

✅ Example Usage
int main() {
    // ZMQ pub address and parent txid to monitor
    const char *zmq_addr = "tcp://127.0.0.1:28332";
    const char *parent_txid = "b2f2e2a1abc1234567890defabcdef1234567890abcdefabcdef1234567890ab";

    monitor_child_tx(zmq_addr, parent_txid);
    return 0;
}

🔄 Integration with Asterisk
Once a child transaction is detected, you can:
  Call an AGI script
  Emit a system signal
  Trigger SIP signaling or audio routing

