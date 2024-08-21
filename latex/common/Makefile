PAPER = main
TEX = $(wildcard *.tex sections/*.tex)
BIB = ref.bib ref.tmp.bib
FIGS = $(wildcard figures/*.pdf figures/*.png graphs/*.pdf graphs/*.png)

.PHONY: all clean

all: $(PAPER).pdf

$(PAPER).pdf: $(TEX) $(BIB) $(FIGS)
	pdflatex -shell-escape $(PAPER)
	bibtex $(PAPER) 
	pdflatex -shell-escape $(PAPER) 
	pdflatex -shell-escape $(PAPER)

clean:
	rm -f *.aux *.bbl *.blg *.log *.out $(PAPER).pdf
