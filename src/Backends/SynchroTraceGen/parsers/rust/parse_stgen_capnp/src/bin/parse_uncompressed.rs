extern crate capnp;
extern crate flate2;

#[allow(non_snake_case)]
pub mod STEventTraceUncompressed_capnp {
  include!(concat!(env!("OUT_DIR"), "/STEventTraceUncompressed_capnp.rs"));
}

use STEventTraceUncompressed_capnp::event_stream_uncompressed::{
    Reader, event::{Comp, Comm, MemType, Sync, SyncType, Marker}};
use capnp::{message, serialize_packed};
use flate2::read::GzDecoder;
use std::{env, fs::File, io::BufRead, io::BufReader, path::PathBuf};


fn br_from_path(fpath: PathBuf) -> Box<BufRead> {
    let ext = {
        let ext = fpath.extension().expect("No extension found for input capnp file").to_os_string();
        ext.into_string().expect(format!("Invalid characters in filename: {}",
                                         fpath.to_string_lossy()).as_str())
    };

    let f = File::open(fpath).unwrap();
    let br: Box<BufRead>  = match ext.as_str() {
        "bin" => Box::new(BufReader::new(f)),
        "gz" => Box::new(BufReader::new(GzDecoder::new(f))),
        _ => panic!(format!("Unexpected extension: {}", ext))
    };

    br
}


fn parse(tracepath: String) -> capnp::Result<()> {
    let mut br = br_from_path(PathBuf::from(tracepath));
    let options = message::ReaderOptions {
        traversal_limit_in_words : 2u64.pow(63),
        ..Default::default()
    };

    while br.fill_buf().unwrap().len() != 0 {
        let message = serialize_packed::read_message(&mut br, options).unwrap();
        let stream = message.get_root::<Reader>()?;

        for event in stream.get_events()? {
            match event.which() {
                Ok(Comp(ev)) => {
                    let _iops = ev.get_iops();
                    let _flops = ev.get_flops();
                    match ev.get_mem() {
                        Ok(MemType::Read) => {
                            let _start_addr = ev.get_start_addr();
                            let _end_addr = ev.get_end_addr();
                        }
                        Ok(MemType::Write) => {
                            let _start_addr = ev.get_start_addr();
                            let _end_addr = ev.get_end_addr();
                        }
                        Ok(MemType::None) => {}
                        Err(capnp::NotInSchema(_)) => panic!("Found unknown memory event!")
                    }
                }
                Ok(Comm(ev)) => {
                    let _producer_event = ev.get_producer_event();
                    let _producer_thread = ev.get_producer_thread();
                    let _start_addr = ev.get_start_addr();
                    let _end_addr = ev.get_start_addr();
                }
                Ok(Sync(ev)) => {
                    let _args = ev.get_args();
                    match ev.get_type() {
                        Ok(SyncType::Spawn) => {},
                        Ok(SyncType::Join) => {},
                        Ok(SyncType::Barrier) => {},
                        Ok(SyncType::Lock) => {},
                        Ok(SyncType::Unlock) => {},
                        /* ... */
                        _ => {}
                    }
                }
                Ok(Marker(ev)) => {
                    let _count = ev.get_count();
                }
                Err(capnp::NotInSchema(_)) => panic!("Found unknown event!")
            }
        }
    }

    Ok(())
}


fn main() {
    let fpath = env::args().nth(1).expect("Missing capnp file path");
    println!("Parsing capnp file: {}", fpath);
    parse(fpath).unwrap()
}
