//!
//! commit_op_log - message op_log based off of commitlog
//!
extern crate commitlog;
extern crate serde;
extern crate serde_json;

use self::commitlog::*;
use self::commitlog::message::MessageSet;
use message;
use message_log::MessageLog;
use std::collections::HashMap;
use std::str;
use std::fs;

#[allow(dead_code)]
pub struct CommitOpLog {
    path: String,
    commit_log: CommitLog,
}

impl MessageLog for CommitOpLog {
    fn append(&mut self, pm: &message::ProtocolMessage) {
        self.commit_log.append_msg(serde_json::to_string(&pm).unwrap()).unwrap();
    }
}

impl CommitOpLog {
    pub fn new(fpath: String) -> CommitOpLog {
        fs::remove_dir_all(&fpath).ok();

        let opts = LogOptions::new(&fpath);
        let commit_log = CommitLog::new(opts).unwrap();

        CommitOpLog {
            path: fpath,
            commit_log: commit_log,
        }
    }

    pub fn from_file(fpath: String) -> CommitOpLog {
        let opts = LogOptions::new(&fpath);
        let commit_log = CommitLog::new(opts).unwrap();

        CommitOpLog {
            path: fpath,
            commit_log: commit_log,
        }
    }

    pub fn to_hash(&self) -> HashMap<u64, message::ProtocolMessage> {
        let mut hash = HashMap::new();
        let mut offset = 0;
        let next_offset = self.commit_log.next_offset();

        while offset < next_offset {
            let messages = self.commit_log.read(offset, ReadLimit::default()).unwrap();

            for message in messages.iter() {
                offset += 1;
                let json_s = str::from_utf8(message.payload()).unwrap();
                hash.insert(message.offset(), message::ProtocolMessage::from_string(json_s));
            }
        }
        hash
    }

    // pub fn read(&mut self, offset: &i32) -> message::ProtocolMessage {
    //     let lck = Arc::clone(&self.log_arc);
    //     let log = lck.lock().unwrap();
    //     let pm = log[&offset].clone();
    //     pm
    // }
}

