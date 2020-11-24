//!
//! commit_op_log - message op_log based off of commitlog
//!
extern crate commitlog;
extern crate serde;
extern crate serde_json;

use self::commitlog::CommitLog;
use self::commitlog::LogOptions;

use message;
use message_log::MessageLog;

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
        let opts = LogOptions::new(&fpath);
        let commit_log = CommitLog::new(opts).unwrap();

        CommitOpLog {
            path: fpath,
            commit_log: commit_log,
        }
    }

    // pub fn from_file(fpath: String) -> CommitOpLog {
    //     let seqno = 0;
    //     let mut l = HashMap::new();
    //     let scopy = fpath.clone();
    //     let tlf = File::open(fpath).unwrap();
    //     let mut reader = BufReader::new(&tlf);
    //     let mut line = String::new();
    //     let mut len = reader.read_line(&mut line).unwrap();
    //     while len > 0 {
    //         let pm = message::ProtocolMessage::from_string(&line);
    //         l.insert(pm.uid, pm);
    //         line.clear();
    //         len = reader.read_line(&mut line).unwrap();
    //     }
    //     let lck = Mutex::new(l);
    //     let arc = Arc::new(lck);
    //     CommitOpLog {
    //         seqno: seqno,
    //         log_arc: arc,
    //         path: scopy,
    //         lf: tlf,
    //     }
    // }
    // pub fn read(&mut self, offset: &i32) -> message::ProtocolMessage {
    //     let lck = Arc::clone(&self.log_arc);
    //     let log = lck.lock().unwrap();
    //     let pm = log[&offset].clone();
    //     pm
    // }
}

