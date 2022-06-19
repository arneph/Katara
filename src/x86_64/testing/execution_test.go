package x86_64

import (
	"fmt"
	"os/exec"
	"strings"
	"testing"
)

func TestExecution(t *testing.T) {
	var stdout strings.Builder
	var stderr strings.Builder
	var cmd = exec.Command("./execution_test_executable")
	cmd.Stdin = strings.NewReader(programInput)
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr
	err := cmd.Run()
	if err != nil {
		t.Fatalf("executable failed: %v", err)
	}
	if stderr.String() != "" {
		t.Fatal("executable wrote to stderr")
	}
	gottenOutput, err := ParseOutput(stdout.String())
	if err != nil {
		t.Fatalf("failed to parse stdout: %v", err)
	}
	if gottenOutput.programOutput != expectedProgramOutput {
		t.Errorf("wrong program output:\n%sexpected:\n%s", gottenOutput.programOutput, expectedProgramOutput)
	}
	if gottenOutput.readBuffer != programInput {
		t.Errorf("wrong read buffer:\n%s\nexpected:\n%s", gottenOutput.readBuffer, programInput)
	}
}

type ParsedOutput struct {
	assembly      string
	machineCode   string
	programOutput string
	readBuffer    string
}

func ParseOutput(output string) (ParsedOutput, error) {
	assembly, err := FindBlock(output, "assembly")
	if err != nil {
		return ParsedOutput{}, err
	}
	machineCode, err := FindBlock(output, "machine code")
	if err != nil {
		return ParsedOutput{}, err
	}
	programOutput, err := FindBlock(output, "program output")
	if err != nil {
		return ParsedOutput{}, err
	}
	readBuffer, err := FindBlock(output, "read buffer")
	if err != nil {
		return ParsedOutput{}, err
	}
	return ParsedOutput{
		assembly:      assembly,
		machineCode:   machineCode,
		programOutput: programOutput,
		readBuffer:    readBuffer,
	}, nil
}

func FindBlock(output, blockName string) (string, error) {
	beginString := "BEGIN " + blockName + "\n"
	endString := "END " + blockName + "\n"
	beginIndex := strings.Index(output, beginString)
	if beginIndex == -1 {
		return "", fmt.Errorf("did not find begin string for block: %s", blockName)
	}
	endIndex := strings.Index(output, endString)
	if endIndex == -1 {
		return "", fmt.Errorf("did not find end string for block: %s", blockName)
	} else if endIndex < beginIndex {
		return "", fmt.Errorf("found end string before begin string for block: %s", blockName)
	}
	return output[beginIndex+len(beginString) : endIndex], nil
}

var programInput = "yankee"

var expectedProgramOutput = `1
1
2
3
5
8
13
21
34
55
89
Hello world!
1234
`
