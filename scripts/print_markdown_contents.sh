# Generate table of contents for a given markdown file

# Uses python md-toc library
python -m pip install md-toc > /dev/null || exit 1

python -m md_toc --in-place --toc-marker '<!--TOC-->' --skip-lines 1 github $1
