//
// Copyright (C) 2024      Alexandre Janniaux <ajanni@videolabs.io>
// Copyright (C) 2026 TheFenrisLycaon
//

/* This should only be in the tests/ final modules but cargo doesn't
 * seem to account them correctly otherwise. */

use std::{
    ffi::{c_char, c_int, c_void},
    marker::FnPtr,
};
use heimdallrs_core::plugin::{heimdall_activate, heimdall_deactivate};

pub struct TestContext<Activate, Deactivate = heimdall_deactivate> {
    pub command_cursor: usize,
    pub commands: Vec<heimdallrs_core::plugin::ModuleProperties>,
    pub open_cb: Option<Activate>,
    pub close_cb: Option<Deactivate>,
}

pub fn load_manifest<Activate, Deactivate>(
    context: &mut TestContext<Activate, Deactivate>,
    heimdall_entry: extern "C" fn(
        heimdall_set_cb: heimdallrs_core::plugin::sys::heimdall_set_cb,
        opaque: *mut c_void,
    ) -> c_int,
) -> i32 {
    use heimdallrs_core::plugin::ModuleProperties;

    unsafe extern "C" fn set_cb<T: Sized + FnPtr>(
        context: *mut c_void,
        _target: *mut c_void,
        propid: c_int,
        mut args: ...
    ) -> c_int {
        let context: *mut TestContext<T> = context as *mut _;
        let opcode = ModuleProperties::try_from(propid);
        println!("PropId: {:?} ({})", opcode, propid);

        let opcode = opcode.unwrap();
        assert!((*context).command_cursor < (*context).commands.len());

        assert_eq!((&(*context).commands)[(*context).command_cursor], opcode);
        (*context).command_cursor += 1;

        if opcode == ModuleProperties::MODULE_CB_OPEN {
            let _name = args.arg::<*const c_char>();
            let func = args.arg::<*mut c_void>();
            assert_ne!(func, std::ptr::null_mut());
            (*context).open_cb = unsafe { Some(std::mem::transmute_copy(&func)) };
        } else if opcode == ModuleProperties::MODULE_CB_CLOSE {
            let _name = args.arg::<*const c_char>();
            let func = args.arg::<*mut c_void>();
            assert_ne!(func, std::ptr::null_mut());
            (*context).close_cb = unsafe { Some(std::mem::transmute_copy(&func)) };
        }

        0
    }

    heimdall_entry(set_cb::<heimdall_activate>, context as *mut _ as *mut c_void)
}
