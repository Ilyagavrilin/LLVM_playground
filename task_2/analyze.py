import re
import argparse
from collections import Counter, defaultdict
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.widgets import Button

sns.set(style="whitegrid")

def extract_instruction_names(filename):
    pattern = re.compile(r'\[INSTR\] #\d+: (\w+)')
    instruction_names = []

    with open(filename, 'r') as file:
        for line in file:
            match = pattern.search(line)
            if match:
                instruction_names.append(match.group(1))

    return instruction_names

def find_repeated_sequences(instruction_names, max_sequence_length=5):
    sequence_counts = defaultdict(Counter)
    total_instructions = len(instruction_names)

    for length in range(1, max_sequence_length + 1):
        for i in range(total_instructions - length + 1):
            sequence = tuple(instruction_names[i:i + length])
            sequence_counts[length][sequence] += 1

    for length in sequence_counts:
        for sequence in sequence_counts[length]:
            sequence_counts[length][sequence] /= total_instructions

    return sequence_counts, total_instructions

def plot_single_sequence(ax, sequence_counts, length, total_instructions, max_bars=15):
    ax.clear()
    sequences, frequencies = zip(*sequence_counts[length].most_common(max_bars))
    bars = ax.bar(range(len(sequences)), frequencies, color=sns.color_palette("viridis", len(sequences)))

    ax.set_xticks(range(len(sequences)))
    ax.set_xticklabels([f'{" -> ".join(seq)}' for seq in sequences], rotation=45, ha='right', fontsize=9)
    ax.grid(True, linestyle='--', linewidth=0.6)
    ax.set_title(f'Frequency of Instruction Sequences of Length {length}', fontsize=14, fontweight='bold')
    ax.set_ylabel('Frequency (Occurrences / Total Instructions)', fontsize=12)
    ax.set_xlabel('Instruction Sequence', fontsize=12)

    for bar, freq in zip(bars, frequencies):
        ax.annotate(f'{freq:.2f}', 
                    xy=(bar.get_x() + bar.get_width() / 2, bar.get_height()),
                    xytext=(0, 5),
                    textcoords='offset points',
                    ha='center', va='bottom', fontsize=10, color='black')



    plt.draw()

def interactive_plot(sequence_counts, total_instructions):
    fig, ax = plt.subplots(figsize=(14, 10))
    current_length = [1]

    def update_plot(event, direction):
        if direction == 'next' and current_length[0] < 5:
            current_length[0] += 1
        elif direction == 'prev' and current_length[0] > 1:
            current_length[0] -= 1
        plot_single_sequence(ax, sequence_counts, current_length[0], total_instructions)

    axprev = plt.axes([0.8, 0.93, 0.08, 0.05])
    axnext = plt.axes([0.89, 0.93, 0.08, 0.05])
    bprev = Button(axprev, 'Previous')
    bnext = Button(axnext, 'Next')

    bprev.on_clicked(lambda event: update_plot(event, 'prev'))
    bnext.on_clicked(lambda event: update_plot(event, 'next'))

    fig.suptitle(f'Total number of instructions: {total_instructions}', fontsize=16, fontweight='bold', y=0.98)
    plot_single_sequence(ax, sequence_counts, current_length[0], total_instructions)
    plt.tight_layout(pad=4.0)
    plt.subplots_adjust(bottom=0.2)
    plt.show()

def print_instruction_counts(instruction_names):
    instruction_counter = Counter(instruction_names)
    print("\nInstruction Counts:")
    print("===================")
    for instruction, count in instruction_counter.most_common():
        print(f"{instruction}: {count}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analyze instruction sequences from a file.")
    parser.add_argument("filename", type=str, help="The name of the file containing instruction data")
    args = parser.parse_args()

    instruction_names = extract_instruction_names(args.filename)
    sequence_counts, total_instructions = find_repeated_sequences(instruction_names)

    print_instruction_counts(instruction_names)

    interactive_plot(sequence_counts, total_instructions)