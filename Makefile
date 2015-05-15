.PHONY: all index dox clean

all: index dox clean

# Compile GitHub Pages content
index:
	jekyll build -q --config config.yml

# assumes gh-pages branch as a subdirectory named doc in the master branch
dox:
	find doxygen \
		-not -name doxygen \
		-and -not -name header.html \
		-and -not -name footer.html | xargs rm -rf
	cd ..; doxygen doc/Doxyfile

# cleans generated jekyll output
clean:
	rm -rf doc
