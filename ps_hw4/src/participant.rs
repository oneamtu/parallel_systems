//!
//! participant.rs
//! Implementation of 2PC participant
//!
extern crate log;
extern crate rand;
extern crate stderrlog;
use message::MessageType;
use message::ProtocolMessage;
// use message::RequestStatus;
use oplog;
use participant::rand::prelude::*;
// use std::collections::HashMap;
// use std::sync::atomic::AtomicI32;
use std::sync::atomic::{AtomicBool, Ordering};
// use std::sync::mpsc;
use std::sync::mpsc::{Receiver, Sender};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

const EXIT_SLEEP_DURATION: Duration = Duration::from_millis(10);

///
/// ParticipantState
/// enum for participant 2PC state machine
///
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ParticipantState {
    Quiescent,
    // TODO ...
}

///
/// Participant
/// structure for maintaining per-participant state
/// and communication/synchronization objects to/from coordinator
///
#[derive(Debug)]
pub struct Participant {
    pub id: i32,
    pub name: String,
    running: Arc<AtomicBool>,
    state: ParticipantState,
    tx: Sender<ProtocolMessage>,
    rx: Receiver<ProtocolMessage>,
    log: oplog::OpLog,
    op_success_prob: f64,
    msg_success_prob: f64,
    successful_ops: usize,
    failed_ops: usize,
    unknown_ops: usize,
}

///
/// Participant
/// implementation of per-participant 2PC protocol
/// Required:
/// 1. new -- ctor
/// 2. pub fn report_status -- reports number of committed/aborted/unknown for each participant
/// 3. pub fn protocol() -- implements participant side protocol
///
impl Participant {
    ///
    /// new()
    ///
    /// Return a new participant, ready to run the 2PC protocol
    /// with the coordinator.
    ///
    /// HINT: you may want to pass some channels or other communication
    ///       objects that enable coordinator->participant and participant->coordinator
    ///       messaging to this ctor.
    /// HINT: you may want to pass some global flags that indicate whether
    ///       the protocol is still running to this constructor. There are other
    ///       ways to communicate this, of course.
    ///
    pub fn new(
        i: i32,
        name: String,
        running: Arc<AtomicBool>,
        tx: Sender<ProtocolMessage>,
        rx: Receiver<ProtocolMessage>,
        logpath: String,
        op_success_prob: f64,
        msg_success_prob: f64,
    ) -> Participant {
        Participant {
            state: ParticipantState::Quiescent,
            id: i,
            name: name,
            running: running,
            tx: tx,
            rx: rx,
            log: oplog::OpLog::new(logpath),
            op_success_prob: op_success_prob,
            msg_success_prob: msg_success_prob,
            successful_ops: 0,
            failed_ops: 0,
            unknown_ops: 0,
        }
    }

    ///
    /// send()
    /// Send a protocol message to the coordinator.
    /// This variant can be assumed to always succeed.
    /// You should make sure your solution works using this
    /// variant before working with the send_unreliable variant.
    ///
    /// HINT: you will need to implement something that does the
    ///       actual sending.
    ///
    pub fn send(&mut self, pm: ProtocolMessage) -> bool {
        let send_result = self.tx.send(pm);

        send_result.is_ok()
    }

    ///
    /// send()
    /// Send a protocol message to the coordinator,
    /// with some probability of success thresholded by the
    /// command line option success_probability [0.0..1.0].
    /// This variant can be assumed to always succeed
    ///
    /// HINT: you will need to implement something that does the
    ///       actual sending, but you can use the threshold
    ///       logic in this implementation below.
    ///
    pub fn send_unreliable(&mut self, pm: ProtocolMessage) -> bool {
        let x: f64 = random();
        let result: bool;
        if x < self.msg_success_prob {
            result = self.send(pm);
        } else {
            result = false;
        }
        result
    }

    ///
    /// perform_operation
    /// perform the operation specified in the 2PC proposal,
    /// with some probability of success/failure determined by the
    /// command-line option success_probability.
    ///
    /// HINT: The code provided here is not complete--it provides some
    ///       tracing infrastructure and the probability logic.
    ///       Your implementation need not preserve the method signature
    ///       (it's ok to add parameters or return something other than
    ///       bool if it's more convenient for your design).
    ///
    pub fn perform_operation(&mut self, request: &ProtocolMessage) {
        trace!("participant::perform_operation");

        let result;

        let x: f64 = random();
        if x > self.op_success_prob {
            result = MessageType::ParticipantVoteAbort;
        } else {
            result = MessageType::ParticipantVoteCommit;
        }

        let vote = ProtocolMessage::generate(
            result,
            request.txid,
            self.name.clone(),
            request.opid,
        );

        self.log.append(&vote);
        let send_succeeded = self.send_unreliable(vote);

        if !send_succeeded {
            info!("participant_{} failed to send {:?}, {:?}", self.id, result, request);
        }

        trace!("exit participant::perform_operation");
    }

    pub fn commit_result(&mut self, request: &ProtocolMessage) {
        trace!("participant::commit_result");

        self.log.append(request);

        trace!("exit participant::commit_result");
    }

    ///
    /// report_status()
    /// report the abort/commit/unknown status (aggregate) of all
    /// transaction requests made by this coordinator before exiting.
    ///
    pub fn report_status(&mut self) {
        println!(
            "participant_{}:\tC:{}\tA:{}\tU:{}",
            self.id, self.successful_ops, self.failed_ops, self.unknown_ops
        );
    }

    ///
    /// wait_for_exit_signal(&mut self)
    /// wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        trace!("participant_{} waiting for exit signal", self.id);

        while self.running.load(Ordering::SeqCst) {
            thread::sleep(EXIT_SLEEP_DURATION);
        }

        trace!("participant_{} exiting", self.id);
    }

    ///
    /// protocol()
    /// Implements the participant side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep handling requests!
    /// HINT: wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self) {
        trace!("Participant_{}::protocol", self.id);

        while self.running.load(Ordering::SeqCst) {
            let protocol_message = self.rx.recv().unwrap();

            match protocol_message.mtype {
                MessageType::CoordinatorPropose => {
                    //new proposal, unknown outcome
                    self.unknown_ops += 1;
                    self.perform_operation(&protocol_message)
                },
                MessageType::CoordinatorCommit => {
                    self.unknown_ops -= 1;
                    self.successful_ops += 1;
                    self.commit_result(&protocol_message);
                },
                MessageType::CoordinatorAbort => {
                    self.unknown_ops -= 1;
                    self.failed_ops += 1;
                    self.commit_result(&protocol_message);
                },
                MessageType::CoordinatorExit => break,
                _ => unreachable!(),
            };
        }

        self.wait_for_exit_signal();
        self.report_status();
    }
}
