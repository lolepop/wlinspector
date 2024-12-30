use anyhow::Result;
use wlinspectorcli::handlers;

fn main() -> Result<()> {
    // env_logger::init();
    let info = handlers::list_window_info()?;
    println!("{info:?}");
    Ok(())
}
