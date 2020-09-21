# Build documentation html from markdown file

pygmentize -S default -f html -a .codehilite > engine/docs/api/styles.css

file=$1
fileNoExtension="${file%%.*}"

# ./scripts/add_markdown_contents.sh $file

python -m markdown \
	-x markdown.extensions.fenced_code \
	-x markdown.extensions.codehilite \
	-x markdown.extensions.toc \
	$file \
	-f "${fileNoExtension}.html"

# true garbage solution to add stylesheet

echo '<link href="styles.css" rel="stylesheet">' >> "${fileNoExtension}.html"

echo "${file} -> ${fileNoExtension}.html"
