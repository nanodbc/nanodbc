.PHONY: all index dox clean commit

all: index dox clean

# compiles GitHub Pages content
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

# commit update
commit:
	git add --all
	git commit -am "Updated documentation."
