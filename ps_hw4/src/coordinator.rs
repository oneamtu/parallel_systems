//!
//! coordinator.rs
//! Implementation of 2PC coordinator
//!
extern crate log;
extern crate rand;
extern crate stderrlog;
use coordinator::rand::prelude::*;
use message;
use message::MessageType;
use message::ProtocolMessage;
use message::RequestStatus;
use oplog;
use std::collections::HashMap;
use std::sync::atomic::AtomicI32;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc;
use std::sync::mpsc::channel;
use std::sync::mpsc::{Receiver, Sender};
use std::sync::Arc;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;

/// CoordinatorState
/// States for 2PC state machine
///
/// TODO: add and/or delete!
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CoordinatorState {
    Quiescent,
    // TODO...
}

/// Coordinator
/// struct maintaining state for coordinator
#[derive(Debug)]
pub struct Coordinator {
    state: CoordinatorState,
    log: oplog::OpLog,
    client_txs: Vec<Sender<ProtocolMessage>>,
    client_rxs: Vec<Receiver<ProtocolMessage>>,
    participant_txs: Vec<Sender<ProtocolMessage>>,
    participant_rxs: Vec<Receiver<ProtocolMessage>>,
    running: Arc<AtomicBool>,
    // TODO: rename
    op_success_prob: f64,
    // TODO: ...
}

///
/// Coordinator
/// implementation of coordinator functionality
/// Required:
/// 1. new -- ctor
/// 2. protocol -- implementation of coordinator side of protocol
/// 3. report_status -- report of aggregate commit/abort/unknown stats on exit.
/// 4. participant_join -- what to do when a participant joins
/// 5. client_join -- what to do when a client joins
///
impl Coordinator {
    ///
    /// new()
    /// Initialize a new coordinator
    ///
    /// <params>
    ///     logpath: directory for log files --> create a new log there.
    ///     r: atomic bool --> still running?
    ///     success_prob --> probability operations/sends succeed
    ///
    pub fn new(logpath: String, running: &Arc<AtomicBool>, success_prob: f64) -> Coordinator {
        Coordinator {
            state: CoordinatorState::Quiescent,
            log: oplog::OpLog::new(logpath),
            op_success_prob: success_prob,
            client_txs: vec![],
            client_rxs: vec![],
            participant_txs: vec![],
            participant_rxs: vec![],
            running: running.clone(),
        }
    }

    ///
    /// client_join()
    /// handle the addition of a new client
    /// HINTS: keep track of any channels involved!
    /// HINT: you'll probably need to change this routine's
    ///       signature to return something!
    ///       (e.g. channel(s) to be used)
    ///
    pub fn client_join(
        &mut self,
        name: String,
    ) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) {
        assert!(self.state == CoordinatorState::Quiescent);

        info!("coordinator::client_join({})", name);

        let (local_tx, remote_rx) = channel();
        let (remote_tx, local_rx) = channel();

        self.client_txs.push(local_tx);
        self.client_rxs.push(local_rx);

        (remote_tx, remote_rx)
    }

    ///
    /// participant_join()
    /// handle the addition of a new participant
    /// HINT: keep track of any channels involved!
    /// HINT: you'll probably need to change this routine's
    ///       signature to return something!
    ///       (e.g. channel(s) to be used)
    ///
    pub fn participant_join(
        &mut self,
        name: String,
    ) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) {
        assert!(self.state == CoordinatorState::Quiescent);

        info!("coordinator::participant_join({})", name);

        let (local_tx, remote_rx) = channel();
        let (remote_tx, local_rx) = channel();

        self.participant_txs.push(local_tx);
        self.participant_rxs.push(local_rx);

        (remote_tx, remote_rx)
    }

    ///
    /// send()
    /// send a message, maybe drop it
    /// HINT: you'll need to do something to implement
    ///       the actual sending!
    ///
    pub fn send(&mut self, sender: &Sender<ProtocolMessage>, pm: ProtocolMessage) -> bool {
        let x: f64 = random();
        let mut result: bool = true;
        if x < self.op_success_prob {

            // TODO: implement actual send
        } else {
            // don't send anything!
            // (simulates failure)
            result = false;
        }
        result
    }

    ///
    /// recv_request()
    /// receive a message from a client
    /// to start off the protocol.
    ///
    pub fn recv_request(&mut self) -> Option<ProtocolMessage> {
        let mut result = Option::None;
        assert!(self.state == CoordinatorState::Quiescent);
        trace!("coordinator::recv_request...");

        // TODO: write me!

        trace!("leaving coordinator::recv_request");
        result
    }

    ///
    /// report_status()
    /// report the abort/commit/unknown status (aggregate) of all
    /// transaction requests made by this coordinator before exiting.
    ///
    pub fn report_status(&mut self) {
        let successful_ops: usize = 0; // TODO!
        let failed_ops: usize = 0; // TODO!
        let unknown_ops: usize = 0; // TODO!
        println!(
            "coordinator:\tC:{}\tA:{}\tU:{}",
            successful_ops, failed_ops, unknown_ops
        );
    }

    ///
    /// protocol()
    /// Implements the coordinator side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep handling requests!
    /// HINT: wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self) {
        // TODO!

        self.report_status();
    }
}
