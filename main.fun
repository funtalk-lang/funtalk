exec(client, bytes) = {
    .send(client, ASYNC (EVAL, (STRING bytes), {
        out = ""
        print(a) = {
            a1 = ADD (STRING a) "\n"
            out = ADD (ADD (ADD out (BYTES 0)) (BYTES (SIZE a1))) a1
        }
        draw(t, color, stroke_width, title, x_label, y_label) = {
            r = STRING (BYTES 1)
            r = ADD r (BYTES color)
            r = ADD r (BYTES (FLOAT stroke_width))
            r = ADD r (BYTES (SIZE t))
            r = ADD r (BYTES (SIZE title))
            r = ADD r (BYTES title)
            r = ADD r (BYTES (SIZE x_label))
            r = ADD r (BYTES x_label)
            r = ADD r (BYTES (SIZE y_label))
            r = ADD r (BYTES y_label)
            arr = ARR t
            f i v = BYTES v
            arr.map(f)
            r = ADD r (REDUCE ADD arr "")
            out = ADD out r
        }
    }, (), (), "out") 10) ; the last number is timeout in milliseconds
}
error(e) = {
    .print(e)
}
main() = {
    port = 8080
    .handle_http(port,
        ("post", "/", "exec"),
    )
    .print(ADD "Server successfully running on port " port)
}
