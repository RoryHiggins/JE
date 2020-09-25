"""
Script to build the docs as markdown
"""
import os
import argparse
import logging
import pathlib

import markdown

TEMPLATE_HTML_START = """
    <!doctype html>

    <html lang="en">
    <head>
    <meta charset="utf-8">
    <style>
    pre { line-height: 125%; margin: 0; }
    td.linenos pre { color: #000000; background-color: #f0f0f0; padding: 0 5px 0 5px; }
    span.linenos { color: #000000; background-color: #f0f0f0; padding: 0 5px 0 5px; }
    td.linenos pre.special { color: #000000; background-color: #ffffc0; padding: 0 5px 0 5px; }
    span.linenos.special { color: #000000; background-color: #ffffc0; padding: 0 5px 0 5px; }
    .codehilite .hll { background-color: #ffffcc }
    .codehilite { background: #f8f8f8; }
    .codehilite .c { color: #408080; font-style: italic } /* Comment */
    .codehilite .err { border: 1px solid #FF0000 } /* Error */
    .codehilite .k { color: #008000; font-weight: bold } /* Keyword */
    .codehilite .o { color: #666666 } /* Operator */
    .codehilite .ch { color: #408080; font-style: italic } /* Comment.Hashbang */
    .codehilite .cm { color: #408080; font-style: italic } /* Comment.Multiline */
    .codehilite .cp { color: #BC7A00 } /* Comment.Preproc */
    .codehilite .cpf { color: #408080; font-style: italic } /* Comment.PreprocFile */
    .codehilite .c1 { color: #408080; font-style: italic } /* Comment.Single */
    .codehilite .cs { color: #408080; font-style: italic } /* Comment.Special */
    .codehilite .gd { color: #A00000 } /* Generic.Deleted */
    .codehilite .ge { font-style: italic } /* Generic.Emph */
    .codehilite .gr { color: #FF0000 } /* Generic.Error */
    .codehilite .gh { color: #000080; font-weight: bold } /* Generic.Heading */
    .codehilite .gi { color: #00A000 } /* Generic.Inserted */
    .codehilite .go { color: #888888 } /* Generic.Output */
    .codehilite .gp { color: #000080; font-weight: bold } /* Generic.Prompt */
    .codehilite .gs { font-weight: bold } /* Generic.Strong */
    .codehilite .gu { color: #800080; font-weight: bold } /* Generic.Subheading */
    .codehilite .gt { color: #0044DD } /* Generic.Traceback */
    .codehilite .kc { color: #008000; font-weight: bold } /* Keyword.Constant */
    .codehilite .kd { color: #008000; font-weight: bold } /* Keyword.Declaration */
    .codehilite .kn { color: #008000; font-weight: bold } /* Keyword.Namespace */
    .codehilite .kp { color: #008000 } /* Keyword.Pseudo */
    .codehilite .kr { color: #008000; font-weight: bold } /* Keyword.Reserved */
    .codehilite .kt { color: #B00040 } /* Keyword.Type */
    .codehilite .m { color: #666666 } /* Literal.Number */
    .codehilite .s { color: #BA2121 } /* Literal.String */
    .codehilite .na { color: #7D9029 } /* Name.Attribute */
    .codehilite .nb { color: #008000 } /* Name.Builtin */
    .codehilite .nc { color: #0000FF; font-weight: bold } /* Name.Class */
    .codehilite .no { color: #880000 } /* Name.Constant */
    .codehilite .nd { color: #AA22FF } /* Name.Decorator */
    .codehilite .ni { color: #999999; font-weight: bold } /* Name.Entity */
    .codehilite .ne { color: #D2413A; font-weight: bold } /* Name.Exception */
    .codehilite .nf { color: #0000FF } /* Name.Function */
    .codehilite .nl { color: #A0A000 } /* Name.Label */
    .codehilite .nn { color: #0000FF; font-weight: bold } /* Name.Namespace */
    .codehilite .nt { color: #008000; font-weight: bold } /* Name.Tag */
    .codehilite .nv { color: #19177C } /* Name.Variable */
    .codehilite .ow { color: #AA22FF; font-weight: bold } /* Operator.Word */
    .codehilite .w { color: #bbbbbb } /* Text.Whitespace */
    .codehilite .mb { color: #666666 } /* Literal.Number.Bin */
    .codehilite .mf { color: #666666 } /* Literal.Number.Float */
    .codehilite .mh { color: #666666 } /* Literal.Number.Hex */
    .codehilite .mi { color: #666666 } /* Literal.Number.Integer */
    .codehilite .mo { color: #666666 } /* Literal.Number.Oct */
    .codehilite .sa { color: #BA2121 } /* Literal.String.Affix */
    .codehilite .sb { color: #BA2121 } /* Literal.String.Backtick */
    .codehilite .sc { color: #BA2121 } /* Literal.String.Char */
    .codehilite .dl { color: #BA2121 } /* Literal.String.Delimiter */
    .codehilite .sd { color: #BA2121; font-style: italic } /* Literal.String.Doc */
    .codehilite .s2 { color: #BA2121 } /* Literal.String.Double */
    .codehilite .se { color: #BB6622; font-weight: bold } /* Literal.String.Escape */
    .codehilite .sh { color: #BA2121 } /* Literal.String.Heredoc */
    .codehilite .si { color: #BB6688; font-weight: bold } /* Literal.String.Interpol */
    .codehilite .sx { color: #008000 } /* Literal.String.Other */
    .codehilite .sr { color: #BB6688 } /* Literal.String.Regex */
    .codehilite .s1 { color: #BA2121 } /* Literal.String.Single */
    .codehilite .ss { color: #19177C } /* Literal.String.Symbol */
    .codehilite .bp { color: #008000 } /* Name.Builtin.Pseudo */
    .codehilite .fm { color: #0000FF } /* Name.Function.Magic */
    .codehilite .vc { color: #19177C } /* Name.Variable.Class */
    .codehilite .vg { color: #19177C } /* Name.Variable.Global */
    .codehilite .vi { color: #19177C } /* Name.Variable.Instance */
    .codehilite .vm { color: #19177C } /* Name.Variable.Magic */
    .codehilite .il { color: #666666 } /* Literal.Number.Integer.Long */
    </style>
    </head>
    <body>
"""

TEMPLATE_HTML_END = """
    </body>
    </html>
"""


MARKDOWN_EXTENSIONS = (
    "markdown.extensions.fenced_code",
    "markdown.extensions.codehilite",
    "markdown.extensions.toc",
)


def main():
    logging.basicConfig(level=logging.INFO)

    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('--src-dir', type=str, default="../engine/docs/src")
    arg_parser.add_argument('--build-dir', type=str, default="../engine/docs/build")

    args = arg_parser.parse_args()

    os.makedirs(args.build_dir, exist_ok=True)

    docs_src_dir = pathlib.Path(args.src_dir).resolve(strict=True)
    docs_build_dir = pathlib.Path(args.build_dir).resolve(strict=True)

    logging.info("%s -> %s", docs_src_dir, docs_build_dir)

    docs_src_filenames = docs_src_dir.rglob('*.md')
    for doc_src_filename in docs_src_filenames:
        with open(doc_src_filename, "r") as doc_src_file:
            doc_src = doc_src_file.read()

        doc_html_body = markdown.markdown(doc_src, extensions=MARKDOWN_EXTENSIONS)
        doc_html = TEMPLATE_HTML_START + doc_html_body + TEMPLATE_HTML_END

        doc_html_relative_filename = doc_src_filename.relative_to(docs_src_dir).with_suffix(".html")
        doc_html_filename = docs_build_dir / doc_html_relative_filename

        os.makedirs(doc_html_filename.parent, exist_ok=True)
        with open(doc_html_filename, "w") as doc_html_file:
            doc_html_file.write(doc_html)

        logging.info("%s -> %s", doc_src_filename, doc_html_filename)


if __name__ == '__main__':
    main()