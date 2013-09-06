index:
	# Compile GitHub Pages content
	cp ../example.cpp ./_includes
	jekyll build --config config.yml

dox:
	# assumes gh-pages branch as a subdirectory named doc in the master branch
	find doxygen -type f -not -name header.html -and -not -name footer.html | xargs rm 
	cd ..; doxygen doc/Doxyfile
