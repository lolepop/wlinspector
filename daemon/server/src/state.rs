use std::collections::HashMap;

use wlinspector_common::Process;

pub (crate) struct State {
    pub processes: HashMap<u32, Process>
}

impl State {
    pub fn new() -> Self {
        Self {
            processes: HashMap::new(),
        }
    }
}