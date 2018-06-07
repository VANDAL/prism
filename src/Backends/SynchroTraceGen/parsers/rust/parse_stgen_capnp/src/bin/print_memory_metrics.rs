extern crate flate2;
extern crate capnp;

#[allow(non_snake_case)]
pub mod STEventTraceUncompressed_capnp {
  include!(concat!(env!("OUT_DIR"), "/STEventTraceUncompressed_capnp.rs"));
}

use flate2::read::GzDecoder;
use capnp::{message, serialize_packed};
use STEventTraceUncompressed_capnp::event_stream_uncompressed::{Reader, event::{Comp, Comm, MemType}};

use std::env;
use std::fs::{File, read_dir};
use std::io::{BufRead, BufReader};
use std::collections::HashMap;


/// Return the appropriate BufRead implementation dependening on the type of file
fn br_from_path(fpath: std::path::PathBuf) -> Box<BufRead> {
    let ext = {
        let ext = fpath.extension().expect("No extension found for input capnp file").to_os_string();
        ext.into_string().expect(format!("Invalid characters in filename: {}",
                                         fpath.to_string_lossy()).as_str())
    };

    let f = File::open(fpath).unwrap();
    match ext.as_str() {
        "bin" => Box::new(BufReader::new(f)),
        "gz" => Box::new(BufReader::new(GzDecoder::new(f))),
        _ => panic!(format!("Unexpected extension: {}", ext))
    }
}

/// Update a map of <address, times accessed>
fn update_addrs_counter(counter: &mut HashMap<u64, u64>, addr: u64) {
    counter.entry(addr).and_modify(|e| { *e += 1 }).or_insert(1);
}

/// Return a vector of (address, times accessed) sorted by times accessed,
/// from a map of <address, times accessed>
fn get_sorted_vec(hmap: &HashMap<u64, u64>) -> Vec<(u64, u64)> {
    let mut addrs_counts: Vec<(u64, u64)> = hmap.iter().map(|(&k, &v)| (k, v)).collect();
    addrs_counts.sort_unstable_by(|a, b| a.1.cmp(&b.1).reverse());
    addrs_counts
}

/// Return how many unique addresses make up the top 90% of all memory accesses
fn ninety_perc(v: &Vec<(u64, u64)>, total: u64) -> u64 {
    let ninety_perc_of_total = (0.9 * total as f64) as u64;
    let mut running_total = 0;
    let mut running_unique = 0;
    for &(_, count) in v {
        running_total += count;
        if running_total > ninety_perc_of_total {
            break;
        }
        running_unique += 1;
    }
    running_unique
}

/// Return global memory entropy for all addresses
fn global_entropy(v: &Vec<(u64, u64)>, total: u64) -> f64 {
    let mut running_entropy = 0f64;
    for &(_, count) in v {
        let prob = (count as f64) * 1f64 / (total as f64);
        running_entropy -= prob * prob.log2();
    }
    running_entropy
}

/// Return local memory entropy (the least significant 10 bits cropped off) for all addresses
fn local_entropy(v: &Vec<(u64, u64)>, total: u64) -> f64 {
    let skip_bits = 10;
    let mut counter = HashMap::new();
    for &(addr, count) in v {
        let local_addr = addr >> skip_bits;
        counter.entry(local_addr).and_modify(|e| { *e += count }).or_insert(count);
    }

    let mut local_entropy = 0f64;
    for (_, count) in counter {
        let prob = (count as f64) * 1f64 / (total as f64);
        local_entropy -= prob * prob.log2();
    }
    local_entropy
}

/// Print a set of memory access metrics from a directory of gzipped capnp synchrotraces
fn parse_capnp(dir: String) -> ::capnp::Result<()> {
    // Get all gz files in dir.
    // Assume they are *.capnp.bin.gz
    let gzs = read_dir(dir).unwrap()
        .map(|p| p.unwrap().path())
        .filter(|p| {
            match p.extension() {
                Some(osstr) => { match osstr.to_str().unwrap() { "gz" => true, _ => false } }
                None => false
            }
        });

    let mut uniq_reads_counter = HashMap::new();
    let mut uniq_writes_counter = HashMap::new();
    let mut total_reads = 0;
    let mut total_writes = 0;

    { // Create a new scope so the closures drop their borrows
        // Convenience closures.
        let mut update_reads = |addr: u64| {
            total_reads += 1;
            update_addrs_counter(&mut uniq_reads_counter, addr);
        };
        let mut update_writes = |addr: u64| {
            total_writes += 1;
            update_addrs_counter(&mut uniq_writes_counter, addr);
        };
        // get aggregate metrics for all threads
        for gz in gzs {
            let mut br = br_from_path(std::path::PathBuf::from(gz));
            let options = message::ReaderOptions {
                traversal_limit_in_words : 2u64.pow(63),
                ..Default::default()
            };
            // keep reading messages until EOF
            while br.fill_buf().unwrap().len() != 0 {
                let m = serialize_packed::read_message(&mut br, options).unwrap();
                let event_stream = m.get_root::<Reader>()?;
                for event in event_stream.get_events()? {
                    match event.which() {
                        Ok(Comp(ev)) => {
                            match ev.get_mem() {
                                Ok(MemType::Read) => {
                                    update_reads(ev.get_start_addr());
                                }
                                Ok(MemType::Write) => {
                                    update_writes(ev.get_start_addr());
                                }
                                Ok(_) => {}
                                Err(::capnp::NotInSchema(_)) => panic!("Found unknown memory event!")
                            }
                        }
                        Ok(Comm(ev)) => {
                            // all comms are treated as a reads
                            update_reads(ev.get_start_addr());
                        }
                        Ok(_) => {}
                        Err(::capnp::NotInSchema(_)) => panic!("Found unknown event!")
                    }
                } // end for each event
            } // end while data still in BufRead
        } // end for each gz
    } // end closure scope

    let uniq_reads           = uniq_reads_counter.len();
    let uniq_reads_sorted    = get_sorted_vec(&uniq_reads_counter);
    let reads_ninety         = ninety_perc(&uniq_reads_sorted, total_reads);
    let global_entropy_reads = global_entropy(&uniq_reads_sorted, total_reads);
    let local_entropy_reads  = local_entropy(&uniq_reads_sorted, total_reads);

    let uniq_writes           = uniq_writes_counter.len();
    let uniq_writes_sorted    = get_sorted_vec(&uniq_writes_counter);
    let writes_ninety         = ninety_perc(&uniq_writes_sorted, total_writes);
    let global_entropy_writes = global_entropy(&uniq_writes_sorted, total_writes);
    let local_entropy_writes  = local_entropy(&uniq_writes_sorted, total_writes);

    println!("Total Num.  Reads : {}", total_reads);
    println!("Total Uniq. Reads : {}", uniq_reads);
    println!("90%   Uniq. Reads : {}", reads_ninety);
    println!("Glob  Ent.  Reads : {}", global_entropy_reads);
    println!("Local Ent.  Reads : {}", local_entropy_reads);
    println!("Total Num.  Writes: {}", total_writes);
    println!("Total Uniq. Writes: {}", uniq_writes);
    println!("90%   Uniq. Writes: {}", writes_ninety);
    println!("Glob  Ent.  Writes: {}", global_entropy_writes);
    println!("Local Ent.  Writes: {}", local_entropy_writes);

    Ok(())
}

fn main() {
    let fpath = env::args().nth(1).expect("Missing capnp dir path");
    println!("Parsing capnp dir: {}", fpath);
    parse_capnp(fpath).unwrap()
}
