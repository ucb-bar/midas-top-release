base_dir = $(abspath .)
generated_dir = $(base_dir)/generated-src

compile:
	sbt "run $(generated_dir) MidasTop MidasTop MidasTop DefaultExampleConfig"
