struct Foo {
    a: i32,
    b: i32,
}

fn main() {
    let a;
    a = Foo { a: 1, b: 2 };

    let b = a.a;
}
