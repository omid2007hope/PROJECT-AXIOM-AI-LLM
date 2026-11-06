#!/usr/bin/env python3
"""
Project Axiom — Foundation Language & Lexicon Corpus Generator
File: core/asset_sample/generate_foundation_corpus.py

Generates a comprehensive ChatML training corpus covering:
1. Alphabet, phonetics, orthography, and letter-sound correspondence.
2. English grammar, syntax, parts of speech, and sentence structure.
3. Morphology, word formation, affixes, roots, and lexical semantics.
4. Lexicography and dictionary entries with definitions and etymology.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def build_alphabet_corpus() -> list[dict[str, str]]:
    """Generates examples on English alphabet, phonetics, and orthography."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain the structure of the English alphabet, including vowel and consonant classifications.",
        "response": """The modern English alphabet consists of twenty-six letters derived from the Latin script:
- Uppercase: A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
- Lowercase: a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z

### Phonetic Classification:
1. **Vowels:** The primary vowel letters are A, E, I, O, and U. The letter Y functions as a vowel in words like 'rhythm' and 'fly', and as a consonant glide in 'yellow'. Vowels represent open vocal tract sounds without air constriction.
2. **Consonants:** The twenty-one consonant letters (B, C, D, F, G, H, J, K, L, M, N, P, Q, R, S, T, V, W, X, Y, Z) involve constriction or closure of the vocal tract.
3. **Voicing:** Consonants are paired by voicing:
   - Voiceless: /p/, /t/, /k/, /f/, /s/, /θ/ ('thin'), /ʃ/ ('ship'), /tʃ/ ('chip')
   - Voiced: /b/, /d/, /ɡ/, /v/, /z/, /ð/ ('this'), /ʒ/ ('measure'), /dʒ/ ('judge')

### Orthographic Peculiarities:
- **Digraphs:** Two letters representing a single sound (e.g., 'th', 'sh', 'ch', 'ph').
- **Silent Letters:** Letters retained from historical etymology that are not pronounced in Modern English (e.g., the 'k' in 'knight', the 'b' in 'subtle', the 'p' in 'pneumonia')."""
    })

    examples_list.append({
        "prompt": "Detail standard English punctuation rules and their syntactic functions.",
        "response": """Punctuation marks govern the cadence, grouping, and syntactic boundaries of English prose:

1. **Terminal Punctuation:**
   - **Period (.):** Signals completion of a declarative or imperative sentence.
   - **Question Mark (?):** Concludes a direct interrogative sentence.
   - **Exclamation Point (!):** Conveys heightened emphasis or emotional exclamation.

2. **Clause Linking & Separation:**
   - **Comma (,):** Separates items in a series, offsets introductory dependent clauses, and joins independent clauses when accompanied by coordinating conjunctions (FANBOYS: for, and, nor, but, or, yet, so).
   - **Semicolon (;):** Connects two closely related independent clauses without coordinating conjunctions, or separates complex list items containing internal commas.
   - **Colon (:):** Introduces an elaboration, quotation, list, or definitive specification following an independent clause.

3. **Parenthetical & Qualifying Marks:**
   - **Em Dash (—):** Creates emphatic interruption or structural shift in thought.
   - **Hyphen (-):** Unifies compound modifiers preceding a noun (e.g., 'state-of-the-art system').
   - **Apostrophe ('):** Indicates possession (e.g., 'developer's tool') or omitted letters in contractions ('can't').
   - **Quotation Marks (""):** Enclose direct speech, verbatim citations, or specialized terms."""
    })

    return examples_list


def build_grammar_corpus() -> list[dict[str, str]]:
    """Generates examples on English grammar, syntax, and sentence mechanics."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Break down the eight traditional parts of speech in English with definitions and examples.",
        "response": """The eight traditional parts of speech form the foundational taxonomy of English syntax:

1. **Noun:** Designates a person, place, entity, idea, or concept.
   - Concrete: 'compiler', 'server', 'copper'
   - Abstract: 'determinism', 'latency', 'entropy'
2. **Pronoun:** Substitutes for a noun or noun phrase to prevent repetition.
   - Classes: Personal ('she', 'it'), Demonstrative ('this', 'those'), Relative ('which', 'who'), Indefinite ('everyone', 'none').
3. **Verb:** Expresses action, condition, or state of existence.
   - Types: Transitive ('allocates memory'), Intransitive ('terminates'), Linking ('is robust').
4. **Adjective:** Modifies or quantifies a noun or pronoun.
   - Example: 'The **asynchronous** worker process processed **three** batches.'
5. **Adverb:** Modifies a verb, adjective, or another adverb, indicating manner, time, degree, or place.
   - Example: 'The pipeline executes **extremely efficiently**.'
6. **Preposition:** Establishes spatial, temporal, or logical relationships between a noun phrase and other clause elements.
   - Examples: 'in', 'through', 'across', 'under', 'between'.
7. **Conjunction:** Connects words, phrases, or clauses.
   - Coordinating: 'and', 'but', 'or'
   - Subordinating: 'because', 'although', 'whereas'
   - Correlative: 'neither... nor', 'either... or'
8. **Interjection:** Expresses spontaneous emotion or exclamation.
   - Examples: 'Halt!', 'Indeed.', 'Alas.'"""
    })

    examples_list.append({
        "prompt": "Analyze sentence structures: simple, compound, complex, and compound-complex.",
        "response": """Sentence classification depends on clause composition (independent and dependent clauses):

### 1. Simple Sentence
Consists of exactly one independent clause with a subject and a finite predicate:
- *Example:* "The hardware timer generated an interrupt."
- *Structure:* [Subject: The hardware timer] + [Predicate: generated an interrupt].

### 2. Compound Sentence
Consists of two or more independent clauses joined by a coordinating conjunction or a semicolon:
- *Example:* "The primary buffer filled to capacity, but the overflow worker redirected the excess packets."
- *Structure:* [Independent Clause 1] + [Conjunction: but] + [Independent Clause 2].

### 3. Complex Sentence
Contains one independent clause and at least one dependent (subordinate) clause:
- *Example:* "Although the cache miss incurred latency, the speculative execution pipeline hid the delay."
- *Structure:* [Subordinate Clause: Although the cache miss incurred latency] + [Independent Clause: the speculative execution pipeline hid the delay].

### 4. Compound-Complex Sentence
Contains two or more independent clauses and at least one dependent clause:
- *Example:* "When the ring buffer saturated, the ingestion thread yielded control, and the downstream consumer flushed the queue."
- *Structure:* [Dependent Clause: When the ring buffer saturated] + [Independent Clause 1: the ingestion thread yielded control] + [Conjunction: and] + [Independent Clause 2: the downstream consumer flushed the queue]."""
    })

    examples_list.append({
        "prompt": "Explain English verb tenses, aspects, voice, and grammatical mood.",
        "response": """English verb mechanics combine tense (temporal location), aspect (temporal contour), voice (agent focus), and mood (modality):

### Temporal Tenses & Aspects (12 Standard Forms):
1. **Simple Present:** 'The scheduler dispatches tasks.'
2. **Present Continuous:** 'The scheduler is dispatching tasks.'
3. **Present Perfect:** 'The scheduler has dispatched tasks.'
4. **Present Perfect Continuous:** 'The scheduler has been dispatching tasks.'
5. **Simple Past:** 'The scheduler dispatched tasks.'
6. **Past Continuous:** 'The scheduler was dispatching tasks.'
7. **Past Perfect:** 'The scheduler had dispatched tasks.'
8. **Past Perfect Continuous:** 'The scheduler had been dispatching tasks.'
9. **Simple Future:** 'The scheduler will dispatch tasks.'
10. **Future Continuous:** 'The scheduler will be dispatching tasks.'
11. **Future Perfect:** 'The scheduler will have dispatched tasks.'
12. **Future Perfect Continuous:** 'The scheduler will have been dispatching tasks.'

### Grammatical Voice:
- **Active Voice:** The grammatical subject performs the action. ('The kernel signaled the thread.')
- **Passive Voice:** The grammatical subject receives the action. ('The thread was signaled by the kernel.')

### Grammatical Mood:
- **Indicative Mood:** Asserts factual statements or questions. ('The memory pool is exhausted.')
- **Imperative Mood:** Issues direct commands or requests. ('Flush the write buffer immediately.')
- **Subjunctive Mood:** Expresses hypothetical scenarios, wishes, or requirements. ('If the pointer were null, execution would abort.')"""
    })

    return examples_list


def build_lexicon_corpus() -> list[dict[str, str]]:
    """Generates examples on morphology, word derivation, and lexical semantics."""
    examples_list: list[dict[str, str]] = []

    examples_list.append({
        "prompt": "Explain morphological word formation: morphemes, affixes, roots, and compounding.",
        "response": """Morphology investigates the internal structure of words and their constituent meaningful units:

### 1. Morpheme Classification:
- **Free Morphemes:** Units capable of standing alone as complete words (e.g., 'light', 'break', 'form').
- **Bound Morphemes:** Units that must attach to other morphemes to realize meaning (e.g., prefixes like 'un-', suffixes like '-able').

### 2. Affixation:
- **Prefixes:** Prepend to base roots to alter semantic direction:
  - 'contra-' (against): 'contradiction'
  - 'trans-' (across): 'transformation'
  - 'hyper-' (excessive): 'hyperactive'
  - 'retro-' (backward): 'retrospective'
- **Suffixes:** Append to base roots, frequently shifting grammatical category:
  - Nominalizers: '-ation' ('simulate' -> 'simulation'), '-ity' ('robust' -> 'robustness')
  - Adjectivalizers: '-ible' / '-able' ('reproduce' -> 'reproducible')
  - Verbalizers: '-ize' / '-ise' ('synchronous' -> 'synchronize')
  - Adverbializers: '-ly' ('deterministic' -> 'deterministically')

### 3. Word Formation Mechanisms:
- **Compounding:** Conjoining two distinct free morphemes ('database', 'framework', 'lockstep').
- **Derivation:** Adding affixes that change word class or core meaning ('compute' -> 'computation' -> 'computational').
- **Inflection:** Adding grammatical markers without altering word class ('compile' -> 'compiled', 'compiling', 'compiles').
- **Blending:** Merging parts of two words ('binary' + 'digit' -> 'bit').
- **Acronymy:** Pronounceable words formed from initial letters ('laser', 'radar')."""
    })

    examples_list.append({
        "prompt": "Examine lexical relationships: synonyms, antonyms, homonyms, and polysemy.",
        "response": """Lexical semantics maps the semantic network connecting vocabulary terms:

### 1. Synonymy & Nuance:
Synonyms share core denotations while diverging in register, connotation, or collocations:
- 'Augment' (formal, technical) vs 'Increase' (neutral) vs 'Boost' (informal).
- 'Concurrently' (precise temporal alignment) vs 'Simultaneously' (absolute point convergence).

### 2. Antonymy Types:
- **Complementary (Binary):** Mutual exclusion with no middle ground ('dead' / 'alive', 'boolean' true / false).
- **Gradable:** Continuous scale admitting intermediate degrees ('hot' / 'warm' / 'cool' / 'cold').
- **Converse (Relational):** Dependent perspective pairs ('parent' / 'child', 'predecessor' / 'successor', 'producer' / 'consumer').

### 3. Homonymy, Homophones & Homographs:
- **Homophones:** Identical sound, distinct spelling and meaning ('byte' vs 'bite', 'serial' vs 'cereal').
- **Homographs:** Identical spelling, distinct sound and meaning ('lead' /liːd/ to guide vs 'lead' /lɛd/ heavy metal).
- **True Homonyms:** Identical spelling and pronunciation with unrelated etymologies ('bank' of river vs financial 'bank').

### 4. Polysemy:
A single word possessing multiple related semantic senses derived from a common root:
- 'Thread': A strand of cotton; a thread of conversation; an independent execution context in computing."""
    })

    return examples_list


def build_dictionary_corpus() -> list[dict[str, str]]:
    """Generates structured dictionary entries across diverse lexical registers."""
    examples_list: list[dict[str, str]] = []

    dictionary_records = [
        ("Deterministic", "/dɪˌtɜː.mɪˈnɪs.tɪk/", "Adjective",
         "Relating to the philosophical or algorithmic doctrine that all events, operations, and outputs are completely determined by previously existing causes or initial states.",
         "From Latin determinare ('to enclose, bound, limit').",
         "A pure function provides deterministic output because identical input arguments always yield identical results without side effects."),
        ("Ephemeral", "/ɪˈfɛm.ər.əl/", "Adjective",
         "Lasting for an extremely brief duration; transitory or fleeting in operational existence.",
         "From Ancient Greek ephemeros ('lasting only one day', from epi 'upon' + hemera 'day').",
         "Ephemeral worker nodes spin up on demand to process burst workloads and terminate immediately upon queue completion."),
        ("Paradigm", "/ˈpær.ə.daɪm/", "Noun",
         "A distinct set of concepts, thought patterns, standards, or structural methodologies that define a legitimate contribution to a field.",
         "From Greek paradeigma ('pattern, model', from para- 'beside' + deiknynai 'to show').",
         "The transition from procedural routines to functional immutability represented a major programming paradigm shift."),
        ("Ubiquitous", "/juːˈbɪk.wɪ.təs/", "Adjective",
         "Existing or being encountered everywhere simultaneously; widespread and omnipresent.",
         "From Latin ubique ('everywhere', from ubi 'where').",
         "TCP/IP remains the ubiquitous foundation of modern distributed telecommunication protocols."),
        ("Heuristic", "/hjʊəˈrɪs.tɪk/", "Noun & Adjective",
         "A practical problem-solving technique or rule of thumb that is not guaranteed to be optimal or perfect, but sufficient for immediate goals.",
         "From Ancient Greek heuriskein ('to discover, find').",
         "The compiler applies an inlining heuristic that balances function call overhead elimination against instruction cache bloat."),
        ("Juxtaposition", "/ˌdʒʌk.stə.pəˈzɪʃ.ən/", "Noun",
         "The act or instance of placing two or more things side by side, especially for comparison or contrast.",
         "From Latin juxta ('near, beside') + positio ('placement').",
         "The clean juxtaposition of asynchronous stream pipelines against synchronous blocking loops highlighted the throughput disparity."),
        ("Resilience", "/rɪˈzɪl.jəns/", "Noun",
         "The capacity of a system, material, or organism to withstand, absorb, or rapidly recover from significant shock, perturbation, or failure.",
         "From Latin resilire ('to rebound, spring back').",
         "Fault tolerance and automated node failover ensure architectural resilience across the cloud cluster."),
        ("Synthesize", "/ˈsɪn.θə.saɪz/", "Verb",
         "To combine diverse elements, concepts, or substances into a unified, coherent whole.",
         "From Ancient Greek synthesis ('composition, putting together').",
         "The neural network synthesizes multi-layer embeddings to produce unified context representations.")
    ]

    for word, phonetics, word_class, definition, etymology, usage_sentence in dictionary_records:
        examples_list.append({
            "prompt": f"Provide the comprehensive dictionary definition, etymology, and usage for '{word}'.",
            "response": f"""### Lexical Entry: **{word}**
* **Pronunciation (IPA):** {phonetics}
* **Part of Speech:** {word_class}
* **Etymology:** {etymology}

### Definition:
{definition}

### Contextual Usage:
> *"{usage_sentence}"*

### Synonyms & Near Equivalents:
- Systematic, rigorous, formal expression."""
        })

    return examples_list


def format_chatml(prompt: str, response: str) -> str:
    """Encodes a QA pair into Qwen ChatML standard conversational format."""
    return f"<|im_start|>user\n{prompt.strip()}<|im_end|>\n<|im_start|>assistant\n{response.strip()}<|im_end|>\n"


def generate_corpus(output_path: Path, repeat_multiplier: int = 1) -> int:
    """Assembles all foundation categories, formats in ChatML, and writes to disk."""
    examples_list: list[dict[str, str]] = []
    examples_list.extend(build_alphabet_corpus())
    examples_list.extend(build_grammar_corpus())
    examples_list.extend(build_lexicon_corpus())
    examples_list.extend(build_dictionary_corpus())

    output_path.parent.mkdir(parents=True, exist_ok=True)

    total_characters = 0
    with open(output_path, "w", encoding="utf-8") as output_file:
        for _ in range(repeat_multiplier):
            for example in examples_list:
                chatml_block = format_chatml(example["prompt"], example["response"])
                output_file.write(chatml_block)
                total_characters += len(chatml_block)

    return total_characters


def main() -> None:
    """Entry point for the foundation language and lexicon corpus generator."""
    argument_parser = argparse.ArgumentParser(
        description="Project Axiom Foundation Language & Lexicon Corpus Generator"
    )
    argument_parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("core/asset_sample/foundation_training_corpus.txt"),
        help="Path to output corpus text file."
    )
    argument_parser.add_argument(
        "--multiplier", "-m",
        type=int,
        default=3,
        help="Repeat count multiplier to scale corpus size."
    )
    parsed_arguments = argument_parser.parse_args()

    print("[Axiom Corpus] Generating Foundation Language & Lexicon corpus...")
    total_bytes = generate_corpus(parsed_arguments.output, parsed_arguments.multiplier)
    print(f"[Axiom Corpus] Successfully generated {parsed_arguments.output} "
          f"({total_bytes / 1024:.1f} KB, multiplier={parsed_arguments.multiplier}).")


if __name__ == "__main__":
    main()

