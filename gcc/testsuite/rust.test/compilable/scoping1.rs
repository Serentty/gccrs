fn main() {
    let x = 1;
    {
        let mut x = true;
        {
            x = false;
        }
    }
    let x = x + 1;
}
