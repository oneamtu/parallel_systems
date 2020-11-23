//!
//! client.rs
//! Implementation of 2PC client
//!
extern crate log;
extern crate stderrlog;
use message;
use message::MessageType;
use std::collections::HashMap;
use std::sync::atomic::{AtomicBool, AtomicI32, Ordering};
use std::sync::mpsc::{Receiver, Sender};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

// static counter for getting unique TXID numbers
static TXID_COUNTER: AtomicI32 = AtomicI32::new(1);
const EXIT_SLEEP_DURATION: Duration = Duration::from_millis(10);

// client state and
// primitives for communicating with
// the coordinator
#[derive(Debug)]
pub struct Client {
    pub id: i32,
    pub name: String,
    running: Arc<AtomicBool>,
    tx: Sender<message::ProtocolMessage>,
    rx: Receiver<message::ProtocolMessage>,
    pending_requests: HashMap<i32, message::ProtocolMessage>,
    successful_ops: usize,
    failed_ops: usize,
    request_id: i32,
}

///
/// client implementation
/// Required:
/// 1. new -- ctor
/// 2. pub fn report_status -- reports number of committed/aborted/unknown
/// 3. pub fn protocol(&mut self, n_requests: i32) -- implements client side protocol
///
impl Client {
    ///
    /// new()
    ///
    /// Return a new client, ready to run the 2PC protocol
    /// with the coordinator.
    ///
    /// HINT: you may want to pass some channels or other communication
    ///       objects that enable coordinator->client and client->coordinator
    ///       messaging to this ctor.
    /// HINT: you may want to pass some global flags that indicate whether
    ///       the protocol is still running to this constructor
    ///
    pub fn new(
        i: i32,
        name: String,
        tx: Sender<message::ProtocolMessage>,
        rx: Receiver<message::ProtocolMessage>,
        running: Arc<AtomicBool>,
    ) -> Client {
        Client {
            id: i,
            name: name,
            running: running,
            tx: tx,
            rx: rx,
            pending_requests: HashMap::new(),
            successful_ops: 0,
            failed_ops: 0,
            request_id: 0,
        }
    }

    ///
    /// send_next_operation(&mut self)
    /// send the next operation to the coordinator
    ///
    pub fn send_next_operation(&mut self) {
        trace!("Client_{}::send_next_operation", self.id);

        // create a new request with a unique TXID.
        let txid = TXID_COUNTER.fetch_add(1, Ordering::SeqCst);

        info!(
            "Client {} request({})->txid:{} called",
            self.id, self.request_id, txid
        );
        let pm = message::ProtocolMessage::generate(
            MessageType::ClientRequest,
            txid,
            self.name.clone(),
            self.request_id,
        );

        self.request_id += 1;

        debug!("client {} send payload {:?}", self.id, pm);

        self.pending_requests.insert(txid, pm.clone());
        self.tx.send(pm).unwrap();

        trace!("Client_{}::exit send_next_operation", self.id);
    }

    ///
    /// recv_result()
    /// Wait for the coordinator to respond with the result for the
    /// last issued request. Note that we assume the coordinator does
    /// not fail in this simulation
    ///
    pub fn recv_result(&mut self) {
        debug!("Client_{}::recv_result", self.id);

        let pm = self.rx.recv().unwrap();

        match pm.mtype {
            MessageType::ClientResultCommit => self.successful_ops += 1,
            MessageType::ClientResultAbort => self.failed_ops += 1,
            MessageType::CoordinatorExit => return,
            _ => panic!("Unknown MessageType for message {:?}", pm),
        }

        let pending_request = self.pending_requests.remove(&pm.txid).unwrap();

        assert!(pm.opid == pending_request.opid);

        debug!("Client_{} received payload {:?}", self.id, pm);
    }

    ///
    /// report_status()
    /// report the abort/commit/unknown status (aggregate) of all
    /// transaction requests made by this client before exiting.
    ///
    pub fn report_status(&mut self) {
        println!(
            "Client_{}:\tC:{}\tA:{}\tU:{}",
            self.id, self.successful_ops, self.failed_ops, self.pending_requests.len()
        );
    }

    ///
    /// wait_for_exit_signal(&mut self)
    /// wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        debug!("Client_{} waiting for exit signal", self.id);

        while self.running.load(Ordering::SeqCst) {
            thread::sleep(EXIT_SLEEP_DURATION);
        }

        trace!("Client_{} exiting", self.id);
    }

    ///
    /// protocol()
    /// Implements the client side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep issuing requests!
    /// HINT: if you've issued all your requests, wait for some kind of
    ///       exit signal before returning from the protocol method!
    ///
    pub fn protocol(&mut self, n_requests: i32) {
        // run the 2PC protocol for each of n_requests
        for _ in 0..n_requests {
            if self.running.load(Ordering::SeqCst) {
                self.send_next_operation();
                self.recv_result();
            }
        }

        // wait for signal to exit
        // and then report status
        self.wait_for_exit_signal();
        self.report_status();
    }
}
