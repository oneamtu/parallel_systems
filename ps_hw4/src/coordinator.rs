//!
//! coordinator.rs
//! Implementation of 2PC coordinator
//!
extern crate log;
extern crate rand;
extern crate stderrlog;
// use coordinator::rand::prelude::*;
use message::MessageType;
use message::ProtocolMessage;
use message::RequestStatus;
use oplog;
use std::collections::HashMap;
// use std::sync::atomic::AtomicI32;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc;
use std::sync::mpsc::channel;
use std::sync::mpsc::{Receiver, Sender};
use std::sync::Arc;
// use std::sync::Mutex;
use std::thread;
use std::time::Duration;

const CLIENT_RECV_SLEEP_DURATION: Duration = Duration::from_nanos(50);
const EXIT_SLEEP_DURATION: Duration = Duration::from_millis(10);
static SENDER_ID: &'static str = "coordinator";

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
    client_channels: HashMap<String, (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)>,
    participant_channels: HashMap<String, (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)>,
    running: Arc<AtomicBool>,
    // TODO: rename
    op_success_prob: f64,
    successful_ops: usize,
    failed_ops: usize,
    unknown_ops: usize,
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
            client_channels: HashMap::new(),
            participant_channels: HashMap::new(),
            running: running.clone(),
            successful_ops: 0,
            failed_ops: 0,
            unknown_ops: 0,
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

        self.client_channels.insert(name, (local_tx, local_rx));

        (remote_tx, remote_rx)
    }

    ///
    /// client_leave()
    /// drop client
    ///
    pub fn client_leave(&mut self, name: &String) {
        info!("coordinator::client_leave({})", name);

        self.client_channels.remove(name);
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

        self.participant_channels.insert(name, (local_tx, local_rx));

        (remote_tx, remote_rx)
    }

    ///
    /// participant_leave()
    /// drop participant
    ///
    pub fn participant_leave(&mut self, name: &String) {
        info!("coordinator::client_leave({})", name);

        self.participant_channels.remove(name);
    }

    ///
    /// send()
    /// send a message, maybe drop it
    /// HINT: you'll need to do something to implement
    ///       the actual sending!
    ///
    // TODO
    // fn send(&mut self, tx: &Sender<ProtocolMessage>, pm: ProtocolMessage) -> bool {
    //     let x: f64 = random();
    //     let mut result: bool = true;
    //     if x < self.op_success_prob {

    //         // TODO: implement actual send
    //     } else {
    //         // don't send anything!
    //         // (simulates failure)
    //         result = false;
    //     }
    //     result
    // }

    ///
    /// recv_client_request()
    /// receive a message from a client
    /// to start off the protocol.
    ///
    fn recv_client_request(&mut self) -> Vec<ProtocolMessage> {
        assert!(self.state == CoordinatorState::Quiescent);

        let requests = self.client_channels.values().filter_map(|(_, rx)| {
            rx.try_recv().ok()
        }).collect();

        trace!(
            "coordinator::recv_request from {} clients... {:?}",
            self.client_channels.len(),
            requests
        );
        requests
    }

    ///
    /// send_coordinator_proposal()
    /// send phase 1 of 2-phase commit - propose
    ///
    fn send_coordinator_proposal(&mut self, request: &ProtocolMessage) {
        debug!("coordinator::propose for {:?}", request);
        // retain successful sends - drop disconnected participants
        self.participant_channels.retain(|name, (tx, _)| {
            let proposal = ProtocolMessage::generate(
                MessageType::CoordinatorPropose,
                request.txid,
                SENDER_ID.to_string(),
                request.opid,
            );
            match tx.send(proposal) {
                Ok(_) => true,
                Err(err) => {
                    error!("{} err/disconnected - {:?}", name, err);
                    false
                }
            }
        });
    }

    ///
    /// recv_participant_vote()
    /// receive phase 1 vote messages from a participant
    ///
    fn recv_participant_vote(&mut self, request: &ProtocolMessage) -> RequestStatus {
        // assert!(self.state == CoordinatorState::Quiescent);
        debug!("coordinator::recv_participant_vote...");

        let mut request_status = RequestStatus::Committed;

        // NOTE - we round-robin through all participants
        // even though we could "optimistically" quit early if we know
        // it will be an abort and broadcast that; but for now let's flush the pipe
        for (participant_name, (_, participant_rx)) in self.participant_channels.iter() {
            //TODO: recv_timeout
            request_status = match participant_rx.recv() {
                Ok(response) => {
                    assert!(response.txid == request.txid);

                    match response.mtype {
                        MessageType::ParticipantVoteCommit => request_status,
                        MessageType::ParticipantVoteAbort => RequestStatus::Aborted,
                        _ => unreachable!(),
                    }
                }
                Err(mpsc::RecvError) => {
                    error!("{} err/disconnected", participant_name);
                    //disconnected
                    // TODO: remove from list
                    // self.participant_rxs.remove(participant_name);
                    // self.participant_txs.remove(participant_name);
                    RequestStatus::Unknown
                }
            }
        }
        info!(
            "coordinator::recv_participant_vote result: {:?}",
            request_status
        );
        request_status
    }

    ///
    /// commit_vote_outcome_to_log()
    /// write the commit to the log after tallying the vote
    ///
    fn commit_vote_outcome_to_log(
        &mut self,
        request: &ProtocolMessage,
        vote_status: RequestStatus,
    ) -> (ProtocolMessage, ProtocolMessage) {
        let coordinator_action = match vote_status {
            RequestStatus::Committed => MessageType::CoordinatorCommit,
            RequestStatus::Aborted => MessageType::CoordinatorAbort,
            RequestStatus::Unknown => MessageType::CoordinatorAbort,
        };
        let coordinator_commit = ProtocolMessage::generate(
            coordinator_action,
            request.txid,
            SENDER_ID.to_string(),
            request.opid,
        );
        self.log.append(&coordinator_commit);
        debug!(
            "coordinator::commit_vote_outcome_to_log {:?}",
            coordinator_commit
        );

        //update stats once transaction is committed to the log
        //NOTE - no unknown ops since the current design means the
        //coordinator never leaves the loop without commiting any
        //outstanding requests
        match coordinator_action {
            MessageType::CoordinatorCommit => self.successful_ops += 1,
            MessageType::CoordinatorAbort => self.failed_ops += 1,
            _ => unreachable!(),
        };

        let client_response = ProtocolMessage::generate(
            match coordinator_action {
                MessageType::CoordinatorCommit => MessageType::ClientResultCommit,
                MessageType::CoordinatorAbort => MessageType::ClientResultAbort,
                _ => unreachable!(),
            },
            request.txid,
            SENDER_ID.to_string(),
            request.opid,
        );
        (client_response, coordinator_commit)
    }

    ///
    /// send_vote_outcome_to_participants()
    /// send result of vote to each participant, so they can commit their op
    ///
    fn send_vote_outcome_to_participants(&mut self, coordinator_commit: ProtocolMessage) {
        debug!(
            "coordinator::send_vote_outcome_to_participants num_participants {} {:?}",
            self.participant_channels.len(),
            coordinator_commit
        );
        // retain successful sends - drop disconnected participants
        self.participant_channels.retain(|name, (tx, _)| {
            match tx.send(coordinator_commit.clone()) {
                Ok(_) => true,
                Err(err) => {
                    error!("{} err/disconnected - {:?}", name, err);
                    false
                }
            }
        });
    }

    ///
    /// send_client_response()
    /// send result of vote to each client, so they know
    ///
    fn send_client_response(&mut self, client_name: &String, client_response: ProtocolMessage) {
        debug!(
            "coordinator::send_client_response {:?} to {}",
            client_response, client_name
        );
        // retain successful sends - drop disconnected participants
        let (client_tx, _) = self.client_channels.get(client_name).unwrap();
        match client_tx.send(client_response) {
            Ok(_) => (),
            Err(err) => {
                error!("{} err/disconnected - {:?}", client_name, err);
                self.client_channels.remove(client_name);
            }
        }
    }

    ///
    /// wait_for_exit_signal(&mut self)
    /// wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        debug!("Coordinator waiting for exit signal");

        for (tx, _) in self.participant_channels.values() {
            let exit_message = ProtocolMessage::generate(
                MessageType::CoordinatorExit,
                -1,
                SENDER_ID.to_string(),
                -1,
            );
            // don't worry about disconnects
            tx.send(exit_message).ok();
        }

        for (tx, _) in self.client_channels.values() {
            let exit_message = ProtocolMessage::generate(
                MessageType::CoordinatorExit,
                -1,
                SENDER_ID.to_string(),
                -1,
            );
            // don't worry about disconnects
            tx.send(exit_message).ok();
        }

        while self.running.load(Ordering::SeqCst) {
            thread::sleep(EXIT_SLEEP_DURATION);
        }

        trace!("Coordinator exiting");
    }

    ///
    /// report_status()
    /// report the abort/commit/unknown status (aggregate) of all
    /// transaction requests made by this coordinator before exiting.
    ///
    fn report_status(&mut self) {
        println!(
            "coordinator:\tC:{}\tA:{}\tU:{}",
            self.successful_ops, self.failed_ops, self.unknown_ops
        );
    }

    ///
    /// protocol()
    /// Implements the coordinator side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep handling requests!
    /// HINT: wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self) {
        while self.running.load(Ordering::SeqCst) {
            let requests = self.recv_client_request();

            if requests.len() == 0 {
                //sleep if we're not going to process anything
                thread::sleep(CLIENT_RECV_SLEEP_DURATION);
                continue
            }

            for request in requests {
                //phase 1
                self.send_coordinator_proposal(&request);
                let vote_status = self.recv_participant_vote(&request);

                //phase 2
                let (client_response, coordinator_commit) =
                    self.commit_vote_outcome_to_log(&request, vote_status);
                self.send_vote_outcome_to_participants(coordinator_commit);
                self.send_client_response(&request.senderid, client_response);
            }
        }

        self.wait_for_exit_signal();
        self.report_status();
    }
}
