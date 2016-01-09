

my @files = <./*>;
for my $file ( @files ) {
	#print "[$file]\n";
	system( '../Debug/example', $file );
}

<STDIN>
