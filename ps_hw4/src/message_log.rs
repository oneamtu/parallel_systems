//!
//! Common interface for the logs
//!

use message;

pub trait MessageLog {
    fn append(&mut self, pm: &message::ProtocolMessage);
}
