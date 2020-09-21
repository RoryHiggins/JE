# Build documentation html files from all markdown files

echo generating docs
for file in engine/docs/api/*.md; do
	./scripts/build_doc_html.sh $file
done
