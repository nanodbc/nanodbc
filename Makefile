all: index dox clean

index:
	# Compile GitHub Pages content
	cp ../example.cpp ./_includes
	jekyll build --config config.yml

dox:
	# assumes gh-pages branch as a subdirectory named doc in the master branch
	find doxygen \
		-not -name doxygen \
		-and -not -name header.html \
		-and -not -name footer.html | xargs rm -rf
	cd ..; doxygen doc/Doxyfile

clean:
	# cleans generated jekyll output
	rm -rf doc
