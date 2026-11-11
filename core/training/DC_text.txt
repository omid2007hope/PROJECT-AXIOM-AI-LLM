Fifth Edition, last update November 06, 2021
2
Lessons In Electric Circuits, Volume I - DC
By Tony R. Kuphaldt
Fifth Edition, last update November 06, 2021
i
©2000-2023, Tony R. Kuphaldt
This book is published under the terms and conditions of the Creative Commons License.
These terms and conditions allow for free copying, distribution, and/or modification of this
document by the general public. The full Creative Commons License text is included in the
last chapter.
As an open and collaboratively developed text, this book is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MER-
CHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the Creative Commons
License for more details.
Available in its entirety as part of the Open Book Project collection at:
https://www.ibiblio.org/kuphaldt/electricCircuits
PRINTING HISTORY
• First Edition: Printed in June of 2000. Plain-ASCII illustrations for universal computer
readability.
• Second Edition: Printed in September of 2000. Illustrations reworked in standard graphic
(eps and jpeg) format. Source files translated to Texinfo format for easy online and printed
publication.
• Third Edition: Equations and tables reworked as graphic images rather than plain-ASCII
text.
• Fourth Edition: Printed in August 2001. Source files translated to SubML format. SubML
is a simple markup language designed to easily convert to other markups like LATEX,
HTML, or DocBook using nothing but search-and-replace substitutions.
• Fifth Edition: Printed in August 2002. New sections added, and error corrections made,
since the fourth edition.
ii
Contents
1
BASIC CONCEPTS OF ELECTRICITY
1
1.1
Static electricity
1
1.2
Conductors, insulators, and electron flow . . . . . . . . . . . . . . . . . . . . . . .
8
1.3
Electric circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
12
1.4
Voltage and current
14
1.5
Resistance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
23
1.6
Voltage and current in a practical circuit . . . . . . . . . . . . . . . . . . . . . . .
27
1.7
Conventional versus electron flow . . . . . . . . . . . . . . . . . . . . . . . . . . .
29
1.8
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
33
2
OHM's LAW
35
2.1
How voltage, current, and resistance relate . . . . . . . . . . . . . . . . . . . . . .
35
2.2
An analogy for Ohm's Law . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
40
2.3
Power in electric circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
42
2.4
Calculating electric power . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
44
2.5
Resistors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
46
2.6
Nonlinear conduction
51
2.7
Circuit wiring . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
56
2.8
Polarity of voltage drops . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
60
2.9
Computer simulation of electric circuits . . . . . . . . . . . . . . . . . . . . . . . .
61
2.10 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
76
3
ELECTRICAL SAFETY
77
3.1
The importance of electrical safety . . . . . . . . . . . . . . . . . . . . . . . . . . .
77
3.2
Physiological effects of electricity . . . . . . . . . . . . . . . . . . . . . . . . . . . .
78
3.3
Shock current path . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
80
3.4
Ohm's Law (again!) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
86
3.5
Safe practices . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
93
3.6
Emergency response . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
96
3.7
Common sources of hazard
98
3.8
Safe circuit design
100
3.9
Safe meter usage . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
106
3.10 Electric shock data . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
116
3.11 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
117
iii
iv
CONTENTS
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
117
4
SCIENTIFIC NOTATION AND METRIC PREFIXES
119
4.1
Scientific notation
119
4.2
Arithmetic with scientific notation . . . . . . . . . . . . . . . . . . . . . . . . . . .
121
4.3
Metric notation . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
123
4.4
Metric prefix conversions
124
4.5
Hand calculator use
125
4.6
Scientific notation in SPICE
126
4.7
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
128
5
SERIES AND PARALLEL CIRCUITS
129
5.1
What are "series" and "parallel" circuits? . . . . . . . . . . . . . . . . . . . . . . .
129
5.2
Simple series circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
132
5.3
Simple parallel circuits
139
5.4
Conductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
144
5.5
Power calculations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
146
5.6
Correct use of Ohm's Law . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
147
5.7
Component failure analysis . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
149
5.8
Building simple resistor circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . .
155
5.9
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
170
6
DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
171
6.1
Voltage divider circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
171
6.2
Kirchhoff's Voltage Law (KVL) . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
179
6.3
Current divider circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
190
6.4
Kirchhoff's Current Law (KCL) . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
193
6.5
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
196
7
SERIES-PARALLEL COMBINATION CIRCUITS
197
7.1
What is a series-parallel circuit? . . . . . . . . . . . . . . . . . . . . . . . . . . . .
197
7.2
Analysis technique . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
200
7.3
Re-drawing complex schematics
208
7.4
Component failure analysis . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
216
7.5
Building series-parallel resistor circuits . . . . . . . . . . . . . . . . . . . . . . . .
221
7.6
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
233
8
DC METERING CIRCUITS
235
8.1
What is a meter? . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
235
8.2
Voltmeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
241
8.3
Voltmeter impact on measured circuit . . . . . . . . . . . . . . . . . . . . . . . . .
246
8.4
Ammeter design
253
8.5
Ammeter impact on measured circuit . . . . . . . . . . . . . . . . . . . . . . . . .
260
8.6
Ohmmeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
264
8.7
High voltage ohmmeters . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
269
8.8
Multimeters . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
277
CONTENTS
v
8.9
Kelvin (4-wire) resistance measurement
281
8.10 Bridge circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
288
8.11 Wattmeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
295
8.12 Creating custom calibration resistances . . . . . . . . . . . . . . . . . . . . . . . .
296
8.13 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
299
9
ELECTRICAL INSTRUMENTATION SIGNALS
301
9.1
Analog and digital signals . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
301
9.2
Voltage signal systems . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
304
9.3
Current signal systems
306
9.4
Tachogenerators
309
9.5
Thermocouples
310
9.6
pH measurement . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
315
9.7
Strain gauges . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
321
9.8
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
328
10 DC NETWORK ANALYSIS
329
10.1 What is network analysis? . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
329
10.2 Branch current method
332
10.3 Mesh current method
341
10.4 Node voltage method
357
10.5 Introduction to network theorems . . . . . . . . . . . . . . . . . . . . . . . . . . .
361
10.6 Millman's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
361
10.7 Superposition Theorem
364
10.8 Thevenin's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
369
10.9 Norton's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
373
10.10Thevenin-Norton equivalencies . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
377
10.11Millman's Theorem revisited . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
379
10.12Maximum Power Transfer Theorem . . . . . . . . . . . . . . . . . . . . . . . . . .
381
10.13∆-Y and Y-∆conversions
383
10.14Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
389
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
390
11 BATTERIES AND POWER SYSTEMS
391
11.1 Electron activity in chemical reactions
391
11.2 Battery construction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
397
11.3 Battery ratings . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
400
11.4 Special-purpose batteries . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
402
11.5 Practical considerations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
406
11.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
408
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
408
vi
CONTENTS
12 PHYSICS OF CONDUCTORS AND INSULATORS
409
12.1 Introduction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
409
12.2 Conductor size
411
12.3 Conductor ampacity
417
12.4 Fuses . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
419
12.5 Specific resistance
426
12.6 Temperature coefficient of resistance
431
12.7 Superconductivity
434
12.8 Insulator breakdown voltage . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
436
12.9 Data
438
12.10Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
438
13 CAPACITORS
439
13.1 Electric fields and capacitance
439
13.2 Capacitors and calculus . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
444
13.3 Factors affecting capacitance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
449
13.4 Series and parallel capacitors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
453
13.5 Practical considerations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
454
13.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
459
14 MAGNETISM AND ELECTROMAGNETISM
461
14.1 Permanent magnets
461
14.2 Electromagnetism
465
14.3 Magnetic units of measurement
467
14.4 Permeability and saturation
470
14.5 Electromagnetic induction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
475
14.6 Mutual inductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
477
14.7 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
479
15 INDUCTORS
481
15.1 Magnetic fields and inductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
481
15.2 Inductors and calculus . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
485
15.3 Factors affecting inductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
491
15.4 Series and parallel inductors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
496
15.5 Practical considerations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
498
15.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
498
16 RC AND L/R TIME CONSTANTS
499
16.1 Electrical transients . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
499
16.2 Capacitor transient response . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
499
16.3 Inductor transient response . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
502
16.4 Voltage and current calculations . . . . . . . . . . . . . . . . . . . . . . . . . . . .
505
16.5 Why L/R and not LR?
511
16.6 Complex voltage and current calculations . . . . . . . . . . . . . . . . . . . . . . .
514
16.7 Complex circuits
515
16.8 Solving for unknown time . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
520
CONTENTS
vii
16.9 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
522
A-1 ABOUT THIS BOOK
523
A-2 CONTRIBUTOR LIST
527
A-3 CC BY License
535
INDEX
538
Chapter 1
BASIC CONCEPTS OF
ELECTRICITY
Contents
1.1
Static electricity
1
1.2
Conductors, insulators, and electron flow
8
1.3
Electric circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 12
1.4
Voltage and current
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 14
1.5
Resistance
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 23
1.6
Voltage and current in a practical circuit
. . . . . . . . . . . . . . . . . . . 27
1.7
Conventional versus electron flow . . . . . . . . . . . . . . . . . . . . . . . . 29
1.8
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 33
1.1
Static electricity
It was discovered centuries ago that certain types of materials would mysteriously attract one
another after being rubbed together. For example: after rubbing a piece of silk against a piece
of glass, the silk and glass would tend to stick together. Indeed, there was an attractive force
that could be demonstrated even when the two materials were separated:
1
2
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Glass rod
Silk cloth
attraction
Glass and silk aren't the only materials known to behave like this. Anyone who has ever
brushed up against a latex balloon only to find that it tries to stick to them has experienced
this same phenomenon. Paraffin wax and wool cloth are another pair of materials early exper-
imenters recognized as manifesting attractive forces after being rubbed together:
attraction
Wool cloth
Wax
This phenomenon became even more interesting when it was discovered that identical ma-
terials, after having been rubbed with their respective cloths, always repelled each other:
1.1.
STATIC ELECTRICITY
3
Glass rod
Glass rod
repulsion
Wax
repulsion
Wax
It was also noted that when a piece of glass rubbed with silk was exposed to a piece of wax
rubbed with wool, the two materials would attract one another:
Glass rod
Wax
attraction
Furthermore, it was found that any material demonstrating properties of attraction or re-
pulsion after being rubbed could be classed into one of two distinct categories: attracted to
glass and repelled by wax, or repelled by glass and attracted to wax. It was either one or the
other: there were no materials found that would be attracted to or repelled by both glass and
wax, or that reacted to one without reacting to the other.
More attention was directed toward the pieces of cloth used to do the rubbing.
It was
discovered that after rubbing two pieces of glass with two pieces of silk cloth, not only did the
glass pieces repel each other, but so did the cloths. The same phenomenon held for the pieces
of wool used to rub the wax:
4
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Silk cloth
Silk cloth
repulsion
repulsion
Wool cloth
Wool cloth
Now, this was really strange to witness. After all, none of these objects were visibly altered
by the rubbing, yet they definitely behaved differently than before they were rubbed. Whatever
change took place to make these materials attract or repel one another was invisible.
Some experimenters speculated that invisible "fluids" were being transferred from one ob-
ject to another during the process of rubbing, and that these "fluids" were able to effect a
physical force over a distance. Charles Dufay was one of the early experimenters who demon-
strated that there were definitely two different types of changes wrought by rubbing certain
pairs of objects together. The fact that there was more than one type of change manifested in
these materials was evident by the fact that there were two types of forces produced: attraction
and repulsion. The hypothetical fluid transfer became known as a charge.
One pioneering researcher, Benjamin Franklin, came to the conclusion that there was only
one fluid exchanged between rubbed objects, and that the two different "charges" were nothing
more than either an excess or a deficiency of that one fluid. After experimenting with wax and
wool, Franklin suggested that the coarse wool removed some of this invisible fluid from the
smooth wax, causing an excess of fluid on the wool and a deficiency of fluid on the wax. The
resulting disparity in fluid content between the wool and wax would then cause an attractive
force, as the fluid tried to regain its former balance between the two materials.
Postulating the existence of a single "fluid" that was either gained or lost through rubbing
accounted best for the observed behavior: that all these materials fell neatly into one of two
categories when rubbed, and most importantly, that the two active materials rubbed against
each other always fell into opposing categories as evidenced by their invariable attraction to
one another. In other words, there was never a time where two materials rubbed against each
other both became either positive or negative.
1.1.
STATIC ELECTRICITY
5
Following Franklin's speculation of the wool rubbing something off of the wax, the type
of charge that was associated with rubbed wax became known as "negative" (because it was
supposed to have a deficiency of fluid) while the type of charge associated with the rubbing
wool became known as "positive" (because it was supposed to have an excess of fluid). Little
did he know that his innocent conjecture would cause much confusion for students of electricity
in the future!
Precise measurements of electrical charge were carried out by the French physicist Charles
Coulomb in the 1780's using a device called a torsional balance measuring the force generated
between two electrically charged objects. The results of Coulomb's work led to the development
of a unit of electrical charge named in his honor, the coulomb. If two "point" objects (hypotheti-
cal objects having no appreciable surface area) were equally charged to a measure of 1 coulomb,
and placed 1 meter (approximately 1 yard) apart, they would generate a force of about 9 billion
newtons (approximately 2 billion pounds), either attracting or repelling depending on the types
of charges involved. The operational definition of a coulomb as the unit of electrical charge (in
terms of force generated between point charges) was found to be equal to an excess or deficiency
of about 6,250,000,000,000,000,000 electrons. Or, stated in reverse terms, one electron has a
charge of about 0.00000000000000000016 coulombs. Being that one electron is the smallest
known carrier of electric charge, this last figure of charge for the electron is defined as the
elementary charge.
It was discovered much later that this "fluid" was actually composed of extremely small bits
of matter called electrons, so named in honor of the ancient Greek word for amber: another
material exhibiting charged properties when rubbed with cloth. Experimentation has since
revealed that all objects are composed of extremely small "building-blocks" known as atoms,
and that these atoms are in turn composed of smaller components known as particles. The
three fundamental particles comprising most atoms are called protons, neutrons and electrons.
Whilst the majority of atoms have a combination of protons, neutrons, and electrons, not all
atoms have neutrons; an example is the protium isotope (1H1) of hydrogen (Hydrogen-1) which
is the lightest and most common form of hydrogen which only has one proton and one electron.
Atoms are far too small to be seen, but if we could look at one, it might appear something like
this:
6
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
= electron
= neutron
e
N
P
P
P
P
P
P
P
N
N
N
N
N
N
e
e
e
e
e
e
Even though each atom in a piece of material tends to hold together as a unit, there's
actually a lot of empty space between the electrons and the cluster of protons and neutrons
residing in the middle.
This crude model is that of the element carbon, with six protons, six neutrons, and six
electrons. In any atom, the protons and neutrons are very tightly bound together, which is an
important quality. The tightly-bound clump of protons and neutrons in the center of the atom
is called the nucleus, and the number of protons in an atom's nucleus determines its elemental
identity: change the number of protons in an atom's nucleus, and you change the type of atom
that it is. In fact, if you could remove three protons from the nucleus of an atom of lead, you
will have achieved the old alchemists' dream of producing an atom of gold! The tight binding
of protons in the nucleus is responsible for the stable identity of chemical elements, and the
failure of alchemists to achieve their dream.
Neutrons are much less influential on the chemical character and identity of an atom than
protons, although they are just as hard to add to or remove from the nucleus, being so tightly
bound. If neutrons are added or gained, the atom will still retain the same chemical iden-
tity, but its mass will change slightly and it may acquire strange nuclear properties such as
radioactivity.
However, electrons have significantly more freedom to move around in an atom than either
protons or neutrons. In fact, they can be knocked out of their respective positions (even leaving
the atom entirely!) by far less energy than what it takes to dislodge particles in the nucleus. If
this happens, the atom still retains its chemical identity, but an important imbalance occurs.
Electrons and protons are unique in the fact that they are attracted to one another over a
distance. It is this attraction over distance which causes the attraction between rubbed objects,
where electrons are moved away from their original atoms to reside around atoms of another
object.
Electrons tend to repel other electrons over a distance, as do protons with other protons.
The only reason protons bind together in the nucleus of an atom is because of a much stronger
force called the strong nuclear force which has effect only under very short distances. Because
1.1.
STATIC ELECTRICITY
7
of this attraction/repulsion behavior between individual particles, electrons and protons are
said to have opposite electric charges. That is, each electron has a negative charge, and each
proton a positive charge. In equal numbers within an atom, they counteract each other's pres-
ence so that the net charge within the atom is zero. This is why the picture of a carbon atom
had six electrons: to balance out the electric charge of the six protons in the nucleus. If elec-
trons leave or extra electrons arrive, the atom's net electric charge will be imbalanced, leaving
the atom "charged" as a whole, causing it to interact with charged particles and other charged
atoms nearby. Neutrons are neither attracted to or repelled by electrons, protons, or even other
neutrons, and are consequently categorized as having no charge at all.
The process of electrons arriving or leaving is exactly what happens when certain combina-
tions of materials are rubbed together: electrons from the atoms of one material are forced by
the rubbing to leave their respective atoms and transfer over to the atoms of the other material.
In other words, electrons comprise the "fluid" hypothesized by Benjamin Franklin.
The result of an imbalance of this "fluid" (electrons) between objects is called static electric-
ity. It is called "static" because the displaced electrons tend to remain stationary after being
moved from one insulating material to another. In the case of wax and wool, it was determined
through further experimentation that electrons in the wool actually transferred to the atoms in
the wax, which is exactly opposite of Franklin's conjecture! In honor of Franklin's designation
of the wax's charge being "negative" and the wool's charge being "positive," electrons are said
to have a "negative" charging influence. Thus, an object whose atoms have received a surplus
of electrons is said to be negatively charged, while an object whose atoms are lacking electrons
is said to be positively charged, as confusing as these designations may seem. By the time the
true nature of electric "fluid" was discovered, Franklin's nomenclature of electric charge was
too well established to be easily changed, and so it remains to this day.
Michael Faraday proved (1832) that static electricity was the same as that produced by a
battery or a generator. Static electricity is, for the most part, a nuisance. Black powder and
smokeless powder have graphite added to prevent ignition due to static electricity. It causes
damage to sensitive semiconductor circuitry. While it is possible to produce motors powered
by high voltage and low current characteristic of static electricity, this is not economic. The
few practical applications of static electricity include xerographic printing, the electrostatic air
filter, and the high voltage Van de Graaff generator.
• REVIEW:
• All materials are made up of tiny "building blocks" known as atoms.
• All naturally occurring atoms contain particles called electrons, protons, and neutrons,
with the exception of the protium isotope (1H1) of hydrogen.
• Electrons have a negative (-) electric charge.
• Protons have a positive (+) electric charge.
• Neutrons have no electric charge.
• Electrons can be dislodged from atoms much easier than protons or neutrons.
• The number of protons in an atom's nucleus determines its identity as a unique element.
8
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
1.2
Conductors, insulators, and electron flow
The electrons of different types of atoms have different degrees of freedom to move around.
With some types of materials, such as metals, the outermost electrons in the atoms are so
loosely bound that they chaotically move in the space between the atoms of that material by
nothing more than the influence of room-temperature heat energy. Because these virtually un-
bound electrons are free to leave their respective atoms and float around in the space between
adjacent atoms, they are often called free electrons.
In other types of materials such as glass, the atoms' electrons have very little freedom to
move around. While external forces such as physical rubbing can force some of these electrons
to leave their respective atoms and transfer to the atoms of another material, they do not move
between atoms within that material very easily.
This relative mobility of electrons within a material is known as electric conductivity. Con-
ductivity is determined by the types of atoms in a material (the number of protons in each
atom's nucleus, determining its chemical identity) and how the atoms are linked together with
one another. Materials with high electron mobility (many free electrons) are called conductors,
while materials with low electron mobility (few or no free electrons) are called insulators.
Here are a few common examples of conductors and insulators:
• Conductors:
• silver
• copper
• gold
• aluminum
• iron
• steel
• brass
• bronze
• mercury
• graphite
• dirty water
• concrete
• Insulators:
• glass
1.2.
CONDUCTORS, INSULATORS, AND ELECTRON FLOW
9
• rubber
• oil
• asphalt
• fiberglass
• porcelain
• ceramic
• quartz
• (dry) cotton
• (dry) paper
• (dry) wood
• plastic
• air
• diamond
• pure water
It must be understood that not all conductive materials have the same level of conductivity,
and not all insulators are equally resistant to electron motion. Electrical conductivity is analo-
gous to the transparency of certain materials to light: materials that easily "conduct" light are
called "transparent," while those that don't are called "opaque." However, not all transparent
materials are equally conductive to light. Window glass is better than most plastics, and cer-
tainly better than "clear" fiberglass. So it is with electrical conductors, some being better than
others.
For instance, silver is the best conductor in the "conductors" list, offering easier passage for
electrons than any other material cited. Dirty water and concrete are also listed as conductors,
but these materials are substantially less conductive than any metal.
It should also be understood that some materials experience changes in their electrical
properties under different conditions. Glass, for instance, is a very good insulator at room
temperature, but becomes a conductor when heated to a very high temperature. Gases such
as air, normally insulating materials, also become conductive if heated to very high tempera-
tures. Most metals become poorer conductors when heated, and better conductors when cooled.
Many conductive materials become perfectly conductive (this is called superconductivity) at ex-
tremely low temperatures.
While the normal motion of "free" electrons in a conductor is random, with no particular
direction or speed, electrons can be influenced to move in a coordinated fashion through a
conductive material. This uniform motion of electrons is what we call electricity, or electric
current. To be more precise, it could be called dynamic electricity in contrast to static electricity,
which is an unmoving accumulation of electric charge. Just like water flowing through the
10
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
emptiness of a pipe, electrons are able to move within the empty space within and between
the atoms of a conductor. The conductor may appear to be solid to our eyes, but any material
composed of atoms is mostly empty space! The liquid-flow analogy is so fitting that the motion
of electrons through a conductor is often referred to as a "flow."
A noteworthy observation may be made here. As each electron moves uniformly through
a conductor, it pushes on the one ahead of it, such that all the electrons move together as a
group. The starting and stopping of electron flow through the length of a conductive path is
virtually instantaneous from one end of a conductor to the other, even though the motion of
each electron may be very slow. An approximate analogy is that of a tube filled end-to-end with
marbles:
Tube
Marble
Marble
The tube is full of marbles, just as a conductor is full of free electrons ready to be moved by
an outside influence. If a single marble is suddenly inserted into this full tube on the left-hand
side, another marble will immediately try to exit the tube on the right. Even though each
marble only traveled a short distance, the transfer of motion through the tube is virtually in-
stantaneous from the left end to the right end, no matter how long the tube is. With electricity,
the overall effect from one end of a conductor to the other happens at the speed of light: a swift
186,000 miles per second!!! Each individual electron, though, travels through the conductor at
a much slower pace.
If we want electrons to flow in a certain direction to a certain place, we must provide the
proper path for them to move, just as a plumber must install piping to get water to flow where
he or she wants it to flow. To facilitate this, wires are made of highly conductive metals such
as copper or aluminum in a wide variety of sizes.
Remember that electrons can flow only when they have the opportunity to move in the
space between the atoms of a material. This means that there can be electric current only
where there exists a continuous path of conductive material providing a conduit for electrons
to travel through. In the marble analogy, marbles can flow into the left-hand side of the tube
(and, consequently, through the tube) if and only if the tube is open on the right-hand side for
marbles to flow out. If the tube is blocked on the right-hand side, the marbles will just "pile
up" inside the tube, and marble "flow" will not occur. The same holds true for electric current:
the continuous flow of electrons requires there be an unbroken path to permit that flow. Let's
look at a diagram to illustrate how this works:
A thin, solid line (as shown above) is the conventional symbol for a continuous piece of wire.
Since the wire is made of a conductive material, such as copper, its constituent atoms have
many free electrons which can easily move through the wire. However, there will never be a
continuous or uniform flow of electrons within this wire unless they have a place to come from
and a place to go. Let's add a hypothetical electron "Source" and "Destination:"
Electron
Electron
Source
Destination
Now, with the Electron Source pushing new electrons into the wire on the left-hand side,
electron flow through the wire can occur (as indicated by the arrows pointing from left to right).
1.2.
CONDUCTORS, INSULATORS, AND ELECTRON FLOW
11
However, the flow will be interrupted if the conductive path formed by the wire is broken:
Electron
Electron
Source
Destination
no flow!
no flow!
(break)
Since air is an insulating material, and an air gap separates the two pieces of wire, the once-
continuous path has now been broken, and electrons cannot flow from Source to Destination.
This is like cutting a water pipe in two and capping off the broken ends of the pipe: water
can't flow if there's no exit out of the pipe. In electrical terms, we had a condition of electrical
continuity when the wire was in one piece, and now that continuity is broken with the wire cut
and separated.
If we were to take another piece of wire leading to the Destination and simply make physical
contact with the wire leading to the Source, we would once again have a continuous path
for electrons to flow. The two dots in the diagram indicate physical (metal-to-metal) contact
between the wire pieces:
Electron
Electron
Source
Destination
no flow!
(break)
Now, we have continuity from the Source, to the newly-made connection, down, to the right,
and up to the Destination. This is analogous to putting a "tee" fitting in one of the capped-off
pipes and directing water through a new segment of pipe to its destination. Please take note
that the broken segment of wire on the right hand side has no electrons flowing through it,
because it is no longer part of a complete path from Source to Destination.
It is interesting to note that no "wear" occurs within wires due to this electric current, unlike
water-carrying pipes which are eventually corroded and worn by prolonged flows. Electrons do
encounter some degree of friction as they move, however, and this friction can generate heat in
a conductor. This is a topic we'll explore in much greater detail later.
• REVIEW:
• In conductive materials, the outer electrons in each atom can easily come or go, and are
called free electrons.
• In insulating materials, the outer electrons are not so free to move.
• All metals are electrically conductive.
• Dynamic electricity, or electric current, is the uniform motion of electrons through a con-
ductor.
• Static electricity is an unmoving (if on an insulator), accumulated charge formed by either
an excess or deficiency of electrons in an object. It is typically formed by charge separation
by contact and separation of dissimilar materials.
• For electrons to flow continuously (indefinitely) through a conductor, there must be a
complete, unbroken path for them to move both into and out of that conductor.
12
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
1.3
Electric circuits
You might have been wondering how electrons can continuously flow in a uniform direction
through wires without the benefit of these hypothetical electron Sources and Destinations.
In order for the Source-and-Destination scheme to work, both would have to have an infinite
capacity for electrons in order to sustain a continuous flow! Using the marble-and-tube analogy,
the marble source and marble destination buckets would have to be infinitely large to contain
enough marble capacity for a "flow" of marbles to be sustained.
The answer to this paradox is found in the concept of a circuit: a never-ending looped
pathway for electrons. If we take a wire, or many wires joined end-to-end, and loop it around
so that it forms a continuous pathway, we have the means to support a uniform flow of electrons
without having to resort to infinite Sources and Destinations:
electrons can flow
in a path without
beginning or end,
continuing forever!
hula-hoop "circuit"
Each electron advancing clockwise in this circuit pushes on the one in front of it, which
pushes on the one in front of it, and so on, and so on, just like a hula-hoop filled with marbles.
Now, we have the capability of supporting a continuous flow of electrons indefinitely without
the need for infinite electron supplies and dumps.
All we need to maintain this flow is a
continuous means of motivation for those electrons, which we'll address in the next section of
this chapter.
It must be realized that continuity is just as important in a circuit as it is in a straight piece
of wire. Just as in the example with the straight piece of wire between the electron Source and
Destination, any break in this circuit will prevent electrons from flowing through it:
1.3.
ELECTRIC CIRCUITS
13
(break)
electron flow cannot
in a "broken" circuit!
no flow!
no flow!
no flow!
occur anywhere
continuous
An important principle to realize here is that it doesn't matter where the break occurs. Any
discontinuity in the circuit will prevent electron flow throughout the entire circuit. Unless
there is a continuous, unbroken loop of conductive material for electrons to flow through, a
sustained flow simply cannot be maintained.
electron flow cannot
in a "broken" circuit!
no flow!
no flow!
no flow!
(break)
occur anywhere
continuous
• REVIEW:
• A circuit is an unbroken loop of conductive material that allows electrons to flow through
continuously without beginning or end.
• If a circuit is "broken," that means its conductive elements no longer form a complete
path, and continuous electron flow cannot occur in it.
• The location of a break in a circuit is irrelevant to its inability to sustain continuous
electron flow. Any break, anywhere in a circuit prevents electron flow throughout the
circuit.
14
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
1.4
Voltage and current
As was previously mentioned, we need more than just a continuous path (circuit) before a con-
tinuous flow of electrons will occur: we also need some means to push these electrons around
the circuit. Just like marbles in a tube or water in a pipe, it takes some kind of influencing
force to initiate flow. With electrons, this force is the same force at work in static electricity:
the force produced by an imbalance of electric charge.
If we take the examples of wax and wool which have been rubbed together, we find that
the surplus of electrons in the wax (negative charge) and the deficit of electrons in the wool
(positive charge) creates an imbalance of charge between them. This imbalance manifests
itself as an attractive force between the two objects:
attraction
Wool cloth
Wax
If a conductive wire is placed between the charged wax and wool, electrons will flow through
it, as some of the excess electrons in the wax rush through the wire to get back to the wool,
filling the deficiency of electrons there:
Wool cloth
Wax
wire
electron flow
The imbalance of electrons between the atoms in the wax and the atoms in the wool creates
a force between the two materials. With no path for electrons to flow from the wax to the
wool, all this force can do is attract the two objects together. Now that a conductor bridges the
insulating gap, however, the force will provoke electrons to flow in a uniform direction through
the wire, if only momentarily, until the charge in that area neutralizes and the force between
the wax and wool diminishes.
The electric charge formed between these two materials by rubbing them together serves to
store a certain amount of energy. This energy is not unlike the energy stored in a high reservoir
of water that has been pumped from a lower-level pond:
1.4.
VOLTAGE AND CURRENT
15
Pump
Pond
Reservoir
Energy stored
Water flow
The influence of gravity on the water in the reservoir creates a force that attempts to move
the water down to the lower level again. If a suitable pipe is run from the reservoir back to the
pond, water will flow under the influence of gravity down from the reservoir, through the pipe:
Pond
Reservoir
Energy released
It takes energy to pump that water from the low-level pond to the high-level reservoir,
and the movement of water through the piping back down to its original level constitutes a
releasing of energy stored from previous pumping.
16
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
If the water is pumped to an even higher level, it will take even more energy to do so, thus
more energy will be stored, and more energy released if the water is allowed to flow through a
pipe back down again:
Reservoir
Pump
Pond
Energy stored
More energy released
More energy stored
Energy released
Reservoir
Pond
Pump
Electrons are not much different. If we rub wax and wool together, we "pump" electrons
1.4.
VOLTAGE AND CURRENT
17
away from their normal "levels," creating a condition where a force exists between the wax
and wool, as the electrons seek to re-establish their former positions (and balance within their
respective atoms). The force attracting electrons back to their original positions around the
positive nuclei of their atoms is analogous to the force gravity exerts on water in the reservoir,
trying to draw it down to its former level.
Just as the pumping of water to a higher level results in energy being stored, "pumping"
electrons to create an electric charge imbalance results in a certain amount of energy being
stored in that imbalance. And, just as providing a way for water to flow back down from the
heights of the reservoir results in a release of that stored energy, providing a way for electrons
to flow back to their original "levels" results in a release of stored energy.
When the electrons are poised in that static condition (just like water sitting still, high in a
reservoir), the energy stored there is called potential energy, because it has the possibility (po-
tential) of release that has not been fully realized yet. When you scuff your rubber-soled shoes
against a fabric carpet on a dry day, you create an imbalance of electric charge between your-
self and the carpet. The action of scuffing your feet stores energy in the form of an imbalance of
electrons forced from their original locations. This charge (static electricity) is stationary, and
you won't realize that energy is being stored at all. However, once you place your hand against
a metal doorknob (with lots of electron mobility to neutralize your electric charge), that stored
energy will be released in the form of a sudden flow of electrons through your hand, and you
will perceive it as an electric shock!
This potential energy, stored in the form of an electric charge imbalance and capable of
provoking electrons to flow through a conductor, can be expressed as a term called voltage,
which technically is a measure of potential energy per unit charge of electrons, or something a
physicist would call specific potential energy. Defined in the context of static electricity, voltage
is the measure of work required to move a unit charge from one location to another, against the
force which tries to keep electric charges balanced. In the context of electrical power sources,
voltage is the amount of potential energy available (work to be done) per unit charge, to move
electrons through a conductor.
Because voltage is an expression of potential energy, representing the possibility or poten-
tial for energy release as the electrons move from one "level" to another, it is always referenced
between two points. Consider the water reservoir analogy:
18
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Reservoir
Location #1
Location #2
Drop
Drop
Because of the difference in the height of the drop, there's potential for much more energy
to be released from the reservoir through the piping to location 2 than to location 1.
The
principle can be intuitively understood in dropping a rock: which results in a more violent
impact, a rock dropped from a height of one foot, or the same rock dropped from a height of
one mile? Obviously, the drop of greater height results in greater energy released (a more
violent impact). We cannot assess the amount of stored energy in a water reservoir simply by
measuring the volume of water any more than we can predict the severity of a falling rock's
impact simply from knowing the weight of the rock: in both cases we must also consider how far
these masses will drop from their initial height. The amount of energy released by allowing
a mass to drop is relative to the distance between its starting and ending points. Likewise,
the potential energy available for moving electrons from one point to another is relative to
those two points. Therefore, voltage is always expressed as a quantity between two points.
Interestingly enough, the analogy of a mass potentially "dropping" from one height to another
is such an apt model that voltage between two points is sometimes called a voltage drop.
Voltage can be generated by means other than rubbing certain types of materials against
each other. Chemical reactions, radiant energy, and the influence of magnetism on conductors
are a few ways in which voltage may be produced. Respective examples of these three sources
of voltage are batteries, solar cells, and generators (such as the "alternator" unit under the
hood of your automobile). For now, we won't go into detail as to how each of these voltage
sources works - more important is that we understand how voltage sources can be applied to
create electron flow in a circuit.
Let's take the symbol for a chemical battery and build a circuit step by step:
1.4.
VOLTAGE AND CURRENT
19
Battery
1
2
Any source of voltage, including batteries, have two points for electrical contact. In this
case, we have point 1 and point 2 in the above diagram. The horizontal lines of varying length
indicate that this is a battery, and they further indicate the direction which this battery's
voltage will try to push electrons through a circuit. The fact that the horizontal lines in the
battery symbol appear separated (and thus unable to serve as a path for electrons to move) is
no cause for concern: in real life, those horizontal lines represent metallic plates immersed in
a liquid or semi-solid material that not only conducts electrons, but also generates the voltage
to push them along by interacting with the plates.
Notice the little "+" and "-" signs to the immediate left of the battery symbol. The negative
(-) end of the battery is always the end with the shortest dash, and the positive (+) end of
the battery is always the end with the longest dash. Since we have decided to call electrons
"negatively" charged (thanks, Ben!), the negative end of a battery is that end which tries to
push electrons out of it. Likewise, the positive end is that end which tries to attract electrons.
With the "+" and "-" ends of the battery not connected to anything, there will be voltage
between those two points, but there will be no flow of electrons through the battery, because
there is no continuous path for the electrons to move.
Battery
1
2
No flow
Pump
Pond
Reservoir
No flow (once the
reservoir has been
completely filled)
Electric Battery
Water analogy
The same principle holds true for the water reservoir and pump analogy: without a return
20
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
pipe back to the pond, stored energy in the reservoir cannot be released in the form of water
flow. Once the reservoir is completely filled up, no flow can occur, no matter how much pressure
the pump may generate. There needs to be a complete path (circuit) for water to flow from the
pond, to the reservoir, and back to the pond in order for continuous flow to occur.
We can provide such a path for the battery by connecting a piece of wire from one end of the
battery to the other. Forming a circuit with a loop of wire, we will initiate a continuous flow of
electrons in a clockwise direction:
Battery
1
2
Pump
Pond
Reservoir
Water analogy
water flow!
electron flow!
water flow!
Electric Circuit
So long as the battery continues to produce voltage and the continuity of the electrical path
1.4.
VOLTAGE AND CURRENT
21
isn't broken, electrons will continue to flow in the circuit. Following the metaphor of water
moving through a pipe, this continuous, uniform flow of electrons through the circuit is called
a current. So long as the voltage source keeps "pushing" in the same direction, the electron flow
will continue to move in the same direction in the circuit. This single-direction flow of electrons
is called a Direct Current, or DC. In the second volume of this book series, electric circuits are
explored where the direction of current switches back and forth: Alternating Current, or AC.
But for now, we'll just concern ourselves with DC circuits.
Because electric current is composed of individual electrons flowing in unison through a
conductor by moving along and pushing on the electrons ahead, just like marbles through a
tube or water through a pipe, the amount of flow throughout a single circuit will be the same
at any point. If we were to monitor a cross-section of the wire in a single circuit, counting the
electrons flowing by, we would notice the exact same quantity per unit of time as in any other
part of the circuit, regardless of conductor length or conductor diameter.
If we break the circuit's continuity at any point, the electric current will cease in the entire
loop, and the full voltage produced by the battery will be manifested across the break, between
the wire ends that used to be connected:
Battery
1
2
(break)
no flow!
no flow!
voltage
drop
Notice the "+" and "-" signs drawn at the ends of the break in the circuit, and how they
correspond to the "+" and "-" signs next to the battery's terminals. These markers indicate the
direction that the voltage attempts to push electron flow, that potential direction commonly
referred to as polarity. Remember that voltage is always relative between two points. Because
of this fact, the polarity of a voltage drop is also relative between two points: whether a point
in a circuit gets labeled with a "+" or a "-" depends on the other point to which it is referenced.
Take a look at the following circuit, where each corner of the loop is marked with a number for
reference:
22
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Battery
1
2
(break)
no flow!
no flow!
3
4
With the circuit's continuity broken between points 2 and 3, the polarity of the voltage
dropped between points 2 and 3 is "-" for point 2 and "+" for point 3. The battery's polarity (1
"-" and 4 "+") is trying to push electrons through the loop clockwise from 1 to 2 to 3 to 4 and
back to 1 again.
Now let's see what happens if we connect points 2 and 3 back together again, but place a
break in the circuit between points 3 and 4:
Battery
1
2
(break)
no flow!
no flow!
3
4
With the break between 3 and 4, the polarity of the voltage drop between those two points
is "+" for 4 and "-" for 3. Take special note of the fact that point 3's "sign" is opposite of that in
the first example, where the break was between points 2 and 3 (where point 3 was labeled "+").
It is impossible for us to say that point 3 in this circuit will always be either "+" or "-", because
polarity, like voltage itself, is not specific to a single point, but is always relative between two
points!
• REVIEW:
• Electrons can be motivated to flow through a conductor by the same force manifested in
static electricity.
• Voltage is the measure of specific potential energy (potential energy per unit charge) be-
tween two locations. In layman's terms, it is the measure of "push" available to motivate
electrons.
• Voltage, as an expression of potential energy, is always relative between two locations, or
points. Sometimes it is called a voltage "drop."
1.5.
RESISTANCE
23
• When a voltage source is connected to a circuit, the voltage will cause a uniform flow of
electrons through that circuit called a current.
• In a single (one loop) circuit, the amount of current at any point is the same as the amount
of current at any other point.
• If a circuit containing a voltage source is broken, the full voltage of that source will appear
across the points of the break.
• The +/- orientation of a voltage drop is called the polarity. It is also relative between two
points.
1.5
Resistance
The circuit in the previous section is not a very practical one. In fact, it can be quite dangerous
to build (directly connecting the poles of a voltage source together with a single piece of wire).
The reason it is dangerous is because the magnitude of electric current may be very large
in such a short circuit, and the release of energy very dramatic (usually in the form of heat).
Usually, electric circuits are constructed in such a way as to make practical use of that released
energy, in as safe a manner as possible.
One practical and popular use of electric current is for the operation of electric lighting. The
simplest form of electric lamp is a tiny metal "filament" inside of a clear glass bulb, which glows
white-hot ("incandesces") with heat energy when sufficient electric current passes through it.
Like the battery, it has two conductive connection points, one for electrons to enter and the
other for electrons to exit.
Connected to a source of voltage, an electric lamp circuit looks something like this:
Battery
electron flow
electron flow
Electric lamp (glowing)
As the electrons work their way through the thin metal filament of the lamp, they encounter
more opposition to motion than they typically would in a thick piece of wire. This opposition to
electric current depends on the type of material, its cross-sectional area, and its temperature.
It is technically known as resistance. (It can be said that conductors have low resistance and
insulators have very high resistance.) This resistance serves to limit the amount of current
through the circuit with a given amount of voltage supplied by the battery, as compared with
24
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
the "short circuit" where we had nothing but a wire joining one end of the voltage source
(battery) to the other.
When electrons move against the opposition of resistance, "friction" is generated. Just like
mechanical friction, the friction produced by electrons flowing against a resistance manifests
itself in the form of heat. The concentrated resistance of a lamp's filament results in a relatively
large amount of heat energy dissipated at that filament. This heat energy is enough to cause
the filament to glow white-hot, producing light, whereas the wires connecting the lamp to the
battery (which have much lower resistance) hardly even get warm while conducting the same
amount of current.
As in the case of the short circuit, if the continuity of the circuit is broken at any point,
electron flow stops throughout the entire circuit. With a lamp in place, this means that it will
stop glowing:
Battery
(break)
no flow!
no flow!
no flow!
voltage
drop
Electric lamp
(not glowing)
As before, with no flow of electrons, the entire potential (voltage) of the battery is available
across the break, waiting for the opportunity of a connection to bridge across that break and
permit electron flow again. This condition is known as an open circuit, where a break in the
continuity of the circuit prevents current throughout. All it takes is a single break in continuity
to "open" a circuit. Once any breaks have been connected once again and the continuity of the
circuit re-established, it is known as a closed circuit.
What we see here is the basis for switching lamps on and off by remote switches. Because
any break in a circuit's continuity results in current stopping throughout the entire circuit, we
can use a device designed to intentionally break that continuity (called a switch), mounted at
any convenient location that we can run wires to, to control the flow of electrons in the circuit:
1.5.
RESISTANCE
25
Battery
switch
It doesn't matter how twisted or
convoluted a route the wires take
conducting current, so long as they
form a complete, uninterrupted
loop (circuit).
This is how a switch mounted on the wall of a house can control a lamp that is mounted
down a long hallway, or even in another room, far away from the switch. The switch itself
is constructed of a pair of conductive contacts (usually made of some kind of metal) forced
together by a mechanical lever actuator or pushbutton. When the contacts touch each other,
electrons are able to flow from one to the other and the circuit's continuity is established; when
the contacts are separated, electron flow from one to the other is prevented by the insulation
of the air between, and the circuit's continuity is broken.
Perhaps the best kind of switch to show for illustration of the basic principle is the "knife"
switch:
A knife switch is nothing more than a conductive lever, free to pivot on a hinge, coming
into physical contact with one or more stationary contact points which are also conductive.
The switch shown in the above illustration is constructed on a porcelain base (an excellent
insulating material), using copper (an excellent conductor) for the "blade" and contact points.
The handle is plastic to insulate the operator's hand from the conductive blade of the switch
when opening or closing it.
Here is another type of knife switch, with two stationary contacts instead of one:
26
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
The particular knife switch shown here has one "blade" but two stationary contacts, mean-
ing that it can make or break more than one circuit. For now this is not terribly important to
be aware of, just the basic concept of what a switch is and how it works.
Knife switches are great for illustrating the basic principle of how a switch works, but they
present distinct safety problems when used in high-power electric circuits. The exposed con-
ductors in a knife switch make accidental contact with the circuit a distinct possibility, and
any sparking that may occur between the moving blade and the stationary contact is free to
ignite any nearby flammable materials. Most modern switch designs have their moving con-
ductors and contact points sealed inside an insulating case in order to mitigate these hazards.
A photograph of a few modern switch types show how the switching mechanisms are much
more concealed than with the knife design:
In keeping with the "open" and "closed" terminology of circuits, a switch that is making
contact from one connection terminal to the other (example: a knife switch with the blade fully
touching the stationary contact point) provides continuity for electrons to flow through, and
1.6.
VOLTAGE AND CURRENT IN A PRACTICAL CIRCUIT
27
is called a closed switch. Conversely, a switch that is breaking continuity (example: a knife
switch with the blade not touching the stationary contact point) won't allow electrons to pass
through and is called an open switch. This terminology is often confusing to the new student
of electronics, because the words "open" and "closed" are commonly understood in the context
of a door, where "open" is equated with free passage and "closed" with blockage. With electrical
switches, these terms have opposite meaning: "open" means no flow while "closed" means free
passage of electrons.
• REVIEW:
• Resistance is the measure of opposition to electric current.
• A short circuit is an electric circuit offering little or no resistance to the flow of electrons.
Short circuits are dangerous with high voltage power sources because the high currents
encountered can cause large amounts of heat energy to be released.
• An open circuit is one where the continuity has been broken by an interruption in the
path for electrons to flow.
• A closed circuit is one that is complete, with good continuity throughout.
• A device designed to open or close a circuit under controlled conditions is called a switch.
• The terms "open" and "closed" refer to switches as well as entire circuits. An open switch
is one without continuity: electrons cannot flow through it. A closed switch is one that
provides a direct (low resistance) path for electrons to flow through.
1.6
Voltage and current in a practical circuit
Because it takes energy to force electrons to flow against the opposition of a resistance, there
will be voltage manifested (or "dropped") between any points in a circuit with resistance be-
tween them. It is important to note that although the amount of current (the quantity of
electrons moving past a given point every second) is uniform in a simple circuit, the amount
of voltage (potential energy per unit charge) between different sets of points in a single circuit
may vary considerably:
28
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Battery
1
2
3
4
same rate of current . . .
. . . at all points in this circuit
Take this circuit as an example. If we label four points in this circuit with the numbers 1,
2, 3, and 4, we will find that the amount of current conducted through the wire between points
1 and 2 is exactly the same as the amount of current conducted through the lamp (between
points 2 and 3). This same quantity of current passes through the wire between points 3 and
4, and through the battery (between points 1 and 4).
However, we will find the voltage appearing between any two of these points to be directly
proportional to the resistance within the conductive path between those two points, given that
the amount of current along any part of the circuit's path is the same (which, for this simple
circuit, it is). In a normal lamp circuit, the resistance of a lamp will be much greater than the
resistance of the connecting wires, so we should expect to see a substantial amount of voltage
between points 2 and 3, with very little between points 1 and 2, or between 3 and 4. The voltage
between points 1 and 4, of course, will be the full amount of "force" offered by the battery, which
will be only slightly greater than the voltage across the lamp (between points 2 and 3).
This, again, is analogous to the water reservoir system:
1.7.
CONVENTIONAL VERSUS ELECTRON FLOW
29
Pump
Pond
Reservoir
Waterwheel
(energy released)
(energy stored)
1
2
3
4
Between points 2 and 3, where the falling water is releasing energy at the water-wheel,
there is a difference of pressure between the two points, reflecting the opposition to the flow
of water through the water-wheel. From point 1 to point 2, or from point 3 to point 4, where
water is flowing freely through reservoirs with little opposition, there is little or no difference
of pressure (no potential energy). However, the rate of water flow in this continuous system is
the same everywhere (assuming the water levels in both pond and reservoir are unchanging):
through the pump, through the water-wheel, and through all the pipes. So it is with simple
electric circuits: the rate of electron flow is the same at every point in the circuit, although
voltages may differ between different sets of points.
1.7
Conventional versus electron flow
"The nice thing about standards is that there are so many of them to choose from."
Andrew S. Tanenbaum, computer science professor
When Benjamin Franklin made his conjecture regarding the direction of charge flow (from
the smooth wax to the rough wool), he set a precedent for electrical notation that exists to
this day, despite the fact that we know electrons are the constituent units of charge, and that
they are displaced from the wool to the wax - not from the wax to the wool - when those
two substances are rubbed together. This is why electrons are said to have a negative charge:
because Franklin assumed electric charge moved in the opposite direction that it actually does,
and so objects he called "negative" (representing a deficiency of charge) actually have a surplus
of electrons.
By the time the true direction of electron flow was discovered, the nomenclature of "posi-
tive" and "negative" had already been so well established in the scientific community that no
effort was made to change it, although calling electrons "positive" would make more sense in
30
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
referring to "excess" charge. You see, the terms "positive" and "negative" are human inventions,
and as such have no absolute meaning beyond our own conventions of language and scientific
description. Franklin could have just as easily referred to a surplus of charge as "black" and a
deficiency as "white," in which case scientists would speak of electrons having a "white" charge
(assuming the same incorrect conjecture of charge position between wax and wool).
However, because we tend to associate the word "positive" with "surplus" and "negative"
with "deficiency," the standard label for electron charge does seem backward. Because of this,
many engineers decided to retain the old concept of electricity with "positive" referring to a sur-
plus of charge, and label charge flow (current) accordingly. This became known as conventional
flow notation:
Conventional flow notation
Electric charge moves
from the positive (surplus)
side of the battery to the
negative (deficiency) side.
Others chose to designate charge flow according to the actual motion of electrons in a circuit.
This form of symbology became known as electron flow notation:
Electric charge moves
side of the battery to the
Electron flow notation
from the negative (surplus)
positive (deficiency) side.
In conventional flow notation, we show the motion of charge according to the (technically
incorrect) labels of + and -. This way the labels make sense, but the direction of charge flow
is incorrect. In electron flow notation, we follow the actual motion of electrons in the circuit,
but the + and - labels seem backward. Does it matter, really, how we designate charge flow
in a circuit? Not really, so long as we're consistent in the use of our symbols. You may follow
an imagined direction of current (conventional flow) or the actual (electron flow) with equal
success insofar as circuit analysis is concerned. Concepts of voltage, current, resistance, conti-
nuity, and even mathematical treatments such as Ohm's Law (chapter 2) and Kirchhoff's Laws
(chapter 6) remain just as valid with either style of notation.
You will find conventional flow notation followed by most electrical engineers, and illus-
trated in most engineering textbooks. Electron flow is most often seen in introductory text-
1.7.
CONVENTIONAL VERSUS ELECTRON FLOW
31
books (this one included) and in the writings of professional scientists, especially solid-state
physicists who are concerned with the actual motion of electrons in substances. These pref-
erences are cultural, in the sense that certain groups of people have found it advantageous to
envision electric current motion in certain ways. Being that most analyses of electric circuits do
not depend on a technically accurate depiction of charge flow, the choice between conventional
flow notation and electron flow notation is arbitrary . . . almost.
Many electrical devices tolerate real currents of either direction with no difference in op-
eration. Incandescent lamps (the type utilizing a thin metal filament that glows white-hot
with sufficient current), for example, produce light with equal efficiency regardless of current
direction. They even function well on alternating current (AC), where the direction changes
rapidly over time. Conductors and switches operate irrespective of current direction, as well.
The technical term for this irrelevance of charge flow is nonpolarization. We could say then,
that incandescent lamps, switches, and wires are nonpolarized components. Conversely, any
device that functions differently on currents of different direction would be called a polarized
device.
There are many such polarized devices used in electric circuits. Most of them are made of so-
called semiconductor substances, and as such aren't examined in detail until the third volume
of this book series. Like switches, lamps, and batteries, each of these devices is represented in a
schematic diagram by a unique symbol. As one might guess, polarized device symbols typically
contain an arrow within them, somewhere, to designate a preferred or exclusive direction of
current. This is where the competing notations of conventional and electron flow really matter.
Because engineers from long ago have settled on conventional flow as their "culture's" standard
notation, and because engineers are the same people who invent electrical devices and the
symbols representing them, the arrows used in these devices' symbols all point in the direction
of conventional flow, not electron flow. That is to say, all of these devices' symbols have arrow
marks that point against the actual flow of electrons through them.
Perhaps the best example of a polarized device is the diode. A diode is a one-way "valve"
for electric current, analogous to a check valve for those familiar with plumbing and hydraulic
systems. Ideally, a diode provides unimpeded flow for current in one direction (little or no
resistance), but prevents flow in the other direction (infinite resistance). Its schematic symbol
looks like this:
Diode
Placed within a battery/lamp circuit, its operation is as such:
32
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Diode operation
Current permitted
Current prohibited
When the diode is facing in the proper direction to permit current, the lamp glows. Other-
wise, the diode blocks all electron flow just like a break in the circuit, and the lamp will not
glow.
If we label the circuit current using conventional flow notation, the arrow symbol of the
diode makes perfect sense: the triangular arrowhead points in the direction of charge flow,
from positive to negative:
Current shown using
conventional flow notation
On the other hand, if we use electron flow notation to show the true direction of electron
travel around the circuit, the diode's arrow symbology seems backward:
Current shown using
electron flow notation
For this reason alone, many people choose to make conventional flow their notation of choice
when drawing the direction of charge motion in a circuit. If for no other reason, the symbols
associated with semiconductor components like diodes make more sense this way. However,
others choose to show the true direction of electron travel so as to avoid having to tell them-
1.8.
CONTRIBUTORS
33
selves, "just remember the electrons are actually moving the other way" whenever the true
direction of electron motion becomes an issue.
In this series of textbooks, I have committed to using electron flow notation. Ironically, this
was not my first choice. I found it much easier when I was first learning electronics to use
conventional flow notation, primarily because of the directions of semiconductor device symbol
arrows. Later, when I began my first formal training in electronics, my instructor insisted on
using electron flow notation in his lectures. In fact, he asked that we take our textbooks (which
were illustrated using conventional flow notation) and use our pens to change the directions of
all the current arrows so as to point the "correct" way! His preference was not arbitrary, though.
In his 20-year career as a U.S. Navy electronics technician, he worked on a lot of vacuum-tube
equipment. Before the advent of semiconductor components like transistors, devices known
as vacuum tubes or electron tubes were used to amplify small electrical signals. These devices
work on the phenomenon of electrons hurtling through a vacuum, their rate of flow controlled
by voltages applied between metal plates and grids placed within their path, and are best
understood when visualized using electron flow notation.
When I graduated from that training program, I went back to my old habit of conventional
flow notation, primarily for the sake of minimizing confusion with component symbols, since
vacuum tubes are all but obsolete except in special applications. Collecting notes for the writ-
ing of this book, I had full intention of illustrating it using conventional flow.
Years later, when I became a teacher of electronics, the curriculum for the program I was
going to teach had already been established around the notation of electron flow. Oddly enough,
this was due in part to the legacy of my first electronics instructor (the 20-year Navy veteran),
but that's another story entirely! Not wanting to confuse students by teaching "differently"
from the other instructors, I had to overcome my habit and get used to visualizing electron flow
instead of conventional. Because I wanted my book to be a useful resource for my students, I
begrudgingly changed plans and illustrated it with all the arrows pointing the "correct" way.
Oh well, sometimes you just can't win!
On a positive note (no pun intended), I have subsequently discovered that some students
prefer electron flow notation when first learning about the behavior of semiconductive sub-
stances. Also, the habit of visualizing electrons flowing against the arrows of polarized device
symbols isn't that difficult to learn, and in the end I've found that I can follow the operation of
a circuit equally well using either mode of notation. Still, I sometimes wonder if it would all be
much easier if we went back to the source of the confusion - Ben Franklin's errant conjecture
- and fixed the problem there, calling electrons "positive" and protons "negative."
1.8
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Bill Heath (September 2002): Pointed out error in illustration of carbon atom - the nucleus
was shown with seven protons instead of six.
Ben Crowell, Ph.D. (January 13, 2001): suggestions on improving the technical accuracy
of voltage and charge definitions.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
34
CHAPTER 1. BASIC CONCEPTS OF ELECTRICITY
Chapter 2
OHM's LAW
Contents
2.1
How voltage, current, and resistance relate . . . . . . . . . . . . . . . . . . 35
2.2
An analogy for Ohm's Law . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 40
2.3
Power in electric circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 42
2.4
Calculating electric power . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 44
2.5
Resistors
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 46
2.6
Nonlinear conduction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 51
2.7
Circuit wiring . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 56
2.8
Polarity of voltage drops . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 60
2.9
Computer simulation of electric circuits . . . . . . . . . . . . . . . . . . . . 61
2.10 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 76
"One microampere flowing in one ohm causes a one microvolt potential drop."
Georg Simon Ohm
2.1
How voltage, current, and resistance relate
An electric circuit is formed when a conductive path is created to allow free electrons to contin-
uously move. This continuous movement of free electrons through the conductors of a circuit
is called a current, and it is often referred to in terms of "flow," just like the flow of a liquid
through a hollow pipe.
The force motivating electrons to "flow" in a circuit is called voltage. Voltage is a specific
measure of potential energy that is always relative between two points. When we speak of a
certain amount of voltage being present in a circuit, we are referring to the measurement of
how much potential energy exists to move electrons from one particular point in that circuit to
another particular point. Without reference to two particular points, the term "voltage" has no
meaning.
35
36
CHAPTER 2. OHM'S LAW
Free electrons tend to move through conductors with some degree of friction, or opposition
to motion. This opposition to motion is more properly called resistance. The amount of current
in a circuit depends on the amount of voltage available to motivate the electrons, and also the
amount of resistance in the circuit to oppose electron flow. Just like voltage, resistance is a
quantity relative between two points. For this reason, the quantities of voltage and resistance
are often stated as being "between" or "across" two points in a circuit.
To be able to make meaningful statements about these quantities in circuits, we need to be
able to describe their quantities in the same way that we might quantify mass, temperature,
volume, length, or any other kind of physical quantity. For mass we might use the units of
"kilogram" or "gram." For temperature we might use degrees Fahrenheit or degrees Celsius.
Here are the standard units of measurement for electrical current, voltage, and resistance:
Quantity
Symbol
Measurement
Unit of
Abbreviation
Unit
Current
Voltage
Resistance
I
E
V
or
R
Ampere ("Amp")
Volt
Ohm
A
V
Ω
The "symbol" given for each quantity is the standard alphabetical letter used to represent
that quantity in an algebraic equation. Standardized letters like these are common in the
disciplines of physics and engineering, and are internationally recognized. The "unit abbrevi-
ation" for each quantity represents the alphabetical symbol used as a shorthand notation for
its particular unit of measurement. And, yes, that strange-looking "horseshoe" symbol is the
capital Greek letter Ω, just a character in a foreign alphabet (apologies to any Greek readers
here).
Each unit of measurement is named after a famous experimenter in electricity: The amp
after the Frenchman Andre M. Ampere, the volt after the Italian Alessandro Volta, and the
ohm after the German Georg Simon Ohm.
The mathematical symbol for each quantity is meaningful as well. The "R" for resistance
and the "V" for voltage are both self-explanatory, whereas "I" for current seems a bit weird.
The "I" is thought to have been meant to represent "Intensity" (of electron flow), and the other
symbol for voltage, "E," stands for "Electromotive force." From what research I've been able
to do, there seems to be some dispute over the meaning of "I." The symbols "E" and "V" are
interchangeable for the most part, although some texts reserve "E" to represent voltage across
a source (such as a battery or generator) and "V" to represent voltage across anything else.
All of these symbols are expressed using capital letters, except in cases where a quantity
(especially voltage or current) is described in terms of a brief period of time (called an "instan-
taneous" value). For example, the voltage of a battery, which is stable over a long period of
time, will be symbolized with a capital letter "E," while the voltage peak of a lightning strike
at the very instant it hits a power line would most likely be symbolized with a lower-case letter
"e" (or lower-case "v") to designate that value as being at a single moment in time. This same
lower-case convention holds true for current as well, the lower-case letter "i" representing cur-
rent at some instant in time. Most direct-current (DC) measurements, however, being stable
over time, will be symbolized with capital letters.
2.1.
HOW VOLTAGE, CURRENT, AND RESISTANCE RELATE
37
One foundational unit of electrical measurement, often taught in the beginnings of electron-
ics courses but used infrequently afterwards, is the unit of the coulomb, which is a measure of
electric charge proportional to the number of electrons in an imbalanced state. One coulomb of
charge is equal to 6,250,000,000,000,000,000 electrons. The symbol for electric charge quantity
is the capital letter "Q," with the unit of coulombs abbreviated by the capital letter "C." It so
happens that the unit for electron flow, the amp, is equal to 1 coulomb of electrons passing by
a given point in a circuit in 1 second of time. Cast in these terms, current is the rate of electric
charge motion through a conductor.
As stated before, voltage is the measure of potential energy per unit charge available to
motivate electrons from one point to another. Before we can precisely define what a "volt"
is, we must understand how to measure this quantity we call "potential energy." The general
metric unit for energy of any kind is the joule, equal to the amount of work performed by a
force of 1 newton exerted through a motion of 1 meter (in the same direction). In British units,
this is slightly less than 3/4 pound of force exerted over a distance of 1 foot. Put in common
terms, it takes about 1 joule of energy to lift a 3/4 pound weight 1 foot off the ground, or to
drag something a distance of 1 foot using a parallel pulling force of 3/4 pound. Defined in these
scientific terms, 1 volt is equal to 1 joule of electric potential energy per (divided by) 1 coulomb
of charge. Thus, a 9 volt battery releases 9 joules of energy for every coulomb of electrons
moved through a circuit.
These units and symbols for electrical quantities will become very important to know as
we begin to explore the relationships between them in circuits. The first, and perhaps most
important, relationship between current, voltage, and resistance is called Ohm's Law, discov-
ered by Georg Simon Ohm and published in his 1827 paper, The Galvanic Circuit Investigated
Mathematically. Ohm's principal discovery was that the amount of electric current through a
metal conductor in a circuit is directly proportional to the voltage impressed across it, for any
given temperature. Ohm expressed his discovery in the form of a simple equation, describing
how voltage, current, and resistance interrelate:
In this algebraic expression, voltage (E) is equal to current (I) multiplied by resistance (R).
Using algebra techniques, we can manipulate this equation into two variations, solving for I
and for R, respectively:
E
R
E
I
Let's see how these equations might work to help us analyze simple circuits:
38
CHAPTER 2. OHM'S LAW
Battery
electron flow
electron flow
Electric lamp (glowing)
In the above circuit, there is only one source of voltage (the battery, on the left) and only one
source of resistance to current (the lamp, on the right). This makes it very easy to apply Ohm's
Law. If we know the values of any two of the three quantities (voltage, current, and resistance)
in this circuit, we can use Ohm's Law to determine the third.
In this first example, we will calculate the amount of current (I) in a circuit, given values of
voltage (E) and resistance (R):
Battery
Lamp
What is the amount of current (I) in this circuit?
E
R
12 V
3 Ω
4 A
In this second example, we will calculate the amount of resistance (R) in a circuit, given
values of voltage (E) and current (I):
2.1.
HOW VOLTAGE, CURRENT, AND RESISTANCE RELATE
39
Battery
Lamp
What is the amount of resistance (R) offered by the lamp?
E
R
I
36 V
4 A
9 Ω
In the last example, we will calculate the amount of voltage supplied by a battery, given
values of current (I) and resistance (R):
Battery
Lamp
What is the amount of voltage provided by the battery?
I
(2 A)(7 Ω) = 14 V
Ohm's Law is a very simple and useful tool for analyzing electric circuits. It is used so
often in the study of electricity and electronics that it needs to be committed to memory by
the serious student. For those who are not yet comfortable with algebra, there's a trick to
remembering how to solve for any one quantity, given the other two. First, arrange the letters
E, I, and R in a triangle like this:
40
CHAPTER 2. OHM'S LAW
E
I
R
If you know E and I, and wish to determine R, just eliminate R from the picture and see
what's left:
E
I
R
E
I
If you know E and R, and wish to determine I, eliminate I and see what's left:
E
I
R
E
R
Lastly, if you know I and R, and wish to determine E, eliminate E and see what's left:
E
I
R
Eventually, you'll have to be familiar with algebra to seriously study electricity and elec-
tronics, but this tip can make your first calculations a little easier to remember. If you are
comfortable with algebra, all you need to do is commit E=IR to memory and derive the other
two formulae from that when you need them!
• REVIEW:
• Voltage measured in volts, symbolized by the letters "E" or "V".
• Current measured in amps, symbolized by the letter "I".
• Resistance measured in ohms, symbolized by the letter "R".
2.2
An analogy for Ohm's Law
Ohm's Law also makes intuitive sense if you apply it to the water-and-pipe analogy. If we have
a water pump that exerts pressure (voltage) to push water around a "circuit" (current) through
2.2.
AN ANALOGY FOR OHM'S LAW
41
a restriction (resistance), we can model how the three variables interrelate. If the resistance to
water flow stays the same and the pump pressure increases, the flow rate must also increase.
Pressure
Flow rate
Resistance
Voltage
Current
Resistance
increase
same
increase
increase
increase
same
If the pressure stays the same and the resistance increases (making it more difficult for the
water to flow), then the flow rate must decrease:
Pressure
Flow rate
Resistance
Voltage
Current
Resistance
same
increase
increase
same
decrease
decrease
If the flow rate were to stay the same while the resistance to flow decreased, the required
pressure from the pump would necessarily decrease:
Pressure
Flow rate
Resistance
Voltage
Current
Resistance
same
same
decrease
decrease
decrease
decrease
decrease
As odd as it may seem, the actual mathematical relationship between pressure, flow, and
resistance is actually more complex for fluids like water than it is for electrons. If you pursue
further studies in physics, you will discover this for yourself. Thankfully for the electronics
student, the mathematics of Ohm's Law is very straightforward and simple.
• REVIEW:
• With resistance steady, current follows voltage (an increase in voltage means an increase
in current, and vice versa).
42
CHAPTER 2. OHM'S LAW
• With voltage steady, changes in current and resistance are opposite (an increase in cur-
rent means a decrease in resistance, and vice versa).
• With current steady, voltage follows resistance (an increase in resistance means an in-
crease in voltage).
2.3
Power in electric circuits
In addition to voltage and current, there is another measure of free electron activity in a circuit:
power. First, we need to understand just what power is before we analyze it in any circuits.
Power is a measure of how much work can be performed in a given amount of time. Work is
generally defined in terms of the lifting of a weight against the pull of gravity. The heavier the
weight and/or the higher it is lifted, the more work has been done. Power is a measure of how
rapidly a standard amount of work is done.
For American automobiles, engine power is rated in a unit called "horsepower," invented
initially as a way for steam engine manufacturers to quantify the working ability of their
machines in terms of the most common power source of their day: horses. One horsepower is
defined in British units as 550 ft-lbs of work per second of time. The power of a car's engine
won't indicate how tall of a hill it can climb or how much weight it can tow, but it will indicate
how fast it can climb a specific hill or tow a specific weight.
The power of a mechanical engine is a function of both the engine's speed and its torque
provided at the output shaft. Speed of an engine's output shaft is measured in revolutions
per minute, or RPM. Torque is the amount of twisting force produced by the engine, and it is
usually measured in pound-feet, or lb-ft (not to be confused with foot-pounds or ft-lbs, which is
the unit for work). Neither speed nor torque alone is a measure of an engine's power.
A 100 horsepower diesel tractor engine will turn relatively slowly, but provide great amounts
of torque. A 100 horsepower motorcycle engine will turn very fast, but provide relatively little
torque. Both will produce 100 horsepower, but at different speeds and different torques. The
equation for shaft horsepower is simple:
Horsepower =
2 π S T
33,000
Where,
S = shaft speed in r.p.m.
T = shaft torque in lb-ft.
Notice how there are only two variable terms on the right-hand side of the equation, S and
T. All the other terms on that side are constant: 2, pi, and 33,000 are all constants (they do not
change in value). The horsepower varies only with changes in speed and torque, nothing else.
We can re-write the equation to show this relationship:
2.3.
POWER IN ELECTRIC CIRCUITS
43
S T
Horsepower
This symbol means
"proportional to"
Because the unit of the "horsepower" doesn't coincide exactly with speed in revolutions per
minute multiplied by torque in pound-feet, we can't say that horsepower equals ST. However,
they are proportional to one another. As the mathematical product of ST changes, the value
for horsepower will change by the same proportion.
In electric circuits, power is a function of both voltage and current. Not surprisingly, this
relationship bears striking resemblance to the "proportional" horsepower formula above:
In this case, however, power (P) is exactly equal to current (I) multiplied by voltage (E),
rather than merely being proportional to IE. When using this formula, the unit of measure-
ment for power is the watt, abbreviated with the letter "W."
It must be understood that neither voltage nor current by themselves constitute power.
Rather, power is the combination of both voltage and current in a circuit. Remember that
voltage is the specific work (or potential energy) per unit charge, while current is the rate at
which electric charges move through a conductor. Voltage (specific work) is analogous to the
work done in lifting a weight against the pull of gravity. Current (rate) is analogous to the
speed at which that weight is lifted. Together as a product (multiplication), voltage (work) and
current (rate) constitute power.
Just as in the case of the diesel tractor engine and the motorcycle engine, a circuit with
high voltage and low current may be dissipating the same amount of power as a circuit with
low voltage and high current. Neither the amount of voltage alone nor the amount of current
alone indicates the amount of power in an electric circuit.
In an open circuit, where voltage is present between the terminals of the source and there
is zero current, there is zero power dissipated, no matter how great that voltage may be. Since
P=IE and I=0 and anything multiplied by zero is zero, the power dissipated in any open circuit
must be zero. Likewise, if we were to have a short circuit constructed of a loop of supercon-
ducting wire (absolutely zero resistance), we could have a condition of current in the loop with
zero voltage, and likewise no power would be dissipated. Since P=IE and E=0 and anything
multiplied by zero is zero, the power dissipated in a superconducting loop must be zero. (We'll
be exploring the topic of superconductivity in a later chapter).
Whether we measure power in the unit of "horsepower" or the unit of "watt," we're still
talking about the same thing: how much work can be done in a given amount of time. The two
units are not numerically equal, but they express the same kind of thing. In fact, European
automobile manufacturers typically advertise their engine power in terms of kilowatts (kW),
or thousands of watts, instead of horsepower! These two units of power are related to each
other by a simple conversion formula:
1 Horsepower = 745.7 Watts
So, our 100 horsepower diesel and motorcycle engines could also be rated as "74570 watt"
engines, or more properly, as "74.57 kilowatt" engines. In European engineering specifications,
44
CHAPTER 2. OHM'S LAW
this rating would be the norm rather than the exception.
• REVIEW:
• Power is the measure of how much work can be done in a given amount of time.
• Mechanical power is commonly measured (in America) in "horsepower."
• Electrical power is almost always measured in "watts," and it can be calculated by the
formula P = IE.
• Electrical power is a product of both voltage and current, not either one separately.
• Horsepower and watts are merely two different units for describing the same kind of
physical measurement, with 1 horsepower equaling 745.7 watts.
2.4
Calculating electric power
We've seen the formula for determining the power in an electric circuit: by multiplying the
voltage in "volts" by the current in "amps" we arrive at an answer in "watts." Let's apply this
to a circuit example:
Battery
Lamp
In the above circuit, we know we have a battery voltage of 18 volts and a lamp resistance of
3 Ω. Using Ohm's Law to determine current, we get:
E
R
18 V
3 Ω
6 A
Now that we know the current, we can take that value and multiply it by the voltage to
determine power:
Answer: the lamp is dissipating (releasing) 108 watts of power, most likely in the form of
both light and heat.
2.4.
CALCULATING ELECTRIC POWER
45
Let's try taking that same circuit and increasing the battery voltage to see what happens.
Intuition should tell us that the circuit current will increase as the voltage increases and the
lamp resistance stays the same. Likewise, the power will increase as well:
Battery
Lamp
Now, the battery voltage is 36 volts instead of 18 volts. The lamp is still providing 3 Ωof
electrical resistance to the flow of electrons. The current is now:
E
R
36 V
3 Ω
12 A
This stands to reason: if I = E/R, and we double E while R stays the same, the current
should double. Indeed, it has: we now have 12 amps of current instead of 6. Now, what about
power?
Notice that the power has increased just as we might have suspected, but it increased quite
a bit more than the current. Why is this? Because power is a function of voltage multiplied
by current, and both voltage and current doubled from their previous values, the power will
increase by a factor of 2 x 2, or 4. You can check this by dividing 432 watts by 108 watts and
seeing that the ratio between them is indeed 4.
Using algebra again to manipulate the formulae, we can take our original power formula
and modify it for applications where we don't know both voltage and current:
If we only know voltage (E) and resistance (R):
If,
E
R
and
Then,
E
R
E
or
E
R
2
If we only know current (I) and resistance (R):
46
CHAPTER 2. OHM'S LAW
If,
I
E
R
and
Then,
or
R
2
I
I R
I
A historical note: it was James Prescott Joule, not Georg Simon Ohm, who first discovered
the mathematical relationship between power dissipation and current through a resistance.
This discovery, published in 1841, followed the form of the last equation (P = I2R), and is
properly known as Joule's Law. However, these power equations are so commonly associated
with the Ohm's Law equations relating voltage, current, and resistance (E=IR ; I=E/R ; and
R=E/I) that they are frequently credited to Ohm.
E
R
E2
I2R
Power equations
• REVIEW:
• Power measured in watts, symbolized by the letter "W".
2.5
Resistors
Because the relationship between voltage, current, and resistance in any circuit is so regular,
we can reliably control any variable in a circuit simply by controlling the other two. Perhaps
the easiest variable in any circuit to control is its resistance. This can be done by changing the
material, size, and shape of its conductive components (remember how the thin metal filament
of a lamp created more electrical resistance than a thick wire?).
Special components called resistors are made for the express purpose of creating a precise
quantity of resistance for insertion into a circuit. They are typically constructed of metal wire
or carbon, and engineered to maintain a stable resistance value over a wide range of environ-
mental conditions. Unlike lamps, they do not produce light, but they do produce heat as electric
power is dissipated by them in a working circuit. Typically, though, the purpose of a resistor is
not to produce usable heat, but simply to provide a precise quantity of electrical resistance.
The most common schematic symbol for a resistor is a zig-zag line:
Resistor values in ohms are usually shown as an adjacent number, and if several resistors
are present in a circuit, they will be labeled with a unique identifier number such as R1, R2,
R3, etc. As you can see, resistor symbols can be shown either horizontally or vertically:
2.5.
RESISTORS
47
with a resistance value
of 150 ohms.
with a resistance value
of 25 ohms.
R1
R2
150
25
This is resistor "R1"
This is resistor "R2"
Real resistors look nothing like the zig-zag symbol. Instead, they look like small tubes or
cylinders with two wires protruding for connection to a circuit. Here is a sampling of different
kinds and sizes of resistors:
In keeping more with their physical appearance, an alternative schematic symbol for a
resistor looks like a small, rectangular box:
Resistors can also be shown to have varying rather than fixed resistances. This might be
for the purpose of describing an actual physical device designed for the purpose of providing
an adjustable resistance, or it could be to show some component that just happens to have an
unstable resistance:
variable
resistance
. . . or . . .
In fact, any time you see a component symbol drawn with a diagonal arrow through it,
that component has a variable rather than a fixed value. This symbol "modifier" (the diagonal
arrow) is standard electronic symbol convention.
Variable resistors must have some physical means of adjustment, either a rotating shaft
or lever that can be moved to vary the amount of electrical resistance. Here is a photograph
48
CHAPTER 2. OHM'S LAW
showing some devices called potentiometers, which can be used as variable resistors:
Because resistors dissipate heat energy as the electric currents through them overcome
the "friction" of their resistance, resistors are also rated in terms of how much heat energy
they can dissipate without overheating and sustaining damage. Naturally, this power rating is
specified in the physical unit of "watts." Most resistors found in small electronic devices such as
portable radios are rated at 1/4 (0.25) watt or less. The power rating of any resistor is roughly
proportional to its physical size. Note in the first resistor photograph how the power ratings
relate with size: the bigger the resistor, the higher its power dissipation rating. Also note how
resistances (in ohms) have nothing to do with size!
Although it may seem pointless now to have a device doing nothing but resisting electric
current, resistors are extremely useful devices in circuits. Because they are simple and so
commonly used throughout the world of electricity and electronics, we'll spend a considerable
amount of time analyzing circuits composed of nothing but resistors and batteries.
For a practical illustration of resistors' usefulness, examine the photograph below. It is a
picture of a printed circuit board, or PCB: an assembly made of sandwiched layers of insulating
phenolic fiber-board and conductive copper strips, into which components may be inserted and
secured by a low-temperature welding process called "soldering." The various components on
this circuit board are identified by printed labels. Resistors are denoted by any label beginning
with the letter "R".
2.5.
RESISTORS
49
This particular circuit board is a computer accessory called a "modem," which allows digital
information transfer over telephone lines. There are at least a dozen resistors (all rated at
1/4 watt power dissipation) that can be seen on this modem's board. Every one of the black
rectangles (called "integrated circuits" or "chips") contain their own array of resistors for their
internal functions, as well.
Another circuit board example shows resistors packaged in even smaller units, called "sur-
face mount devices." This particular circuit board is the underside of a personal computer hard
disk drive, and once again the resistors soldered onto it are designated with labels beginning
with the letter "R":
50
CHAPTER 2. OHM'S LAW
There are over one hundred surface-mount resistors on this circuit board, and this count
of course does not include the number of resistors internal to the black "chips." These two
photographs should convince anyone that resistors - devices that "merely" oppose the flow of
electrons - are very important components in the realm of electronics!
In schematic diagrams, resistor symbols are sometimes used to illustrate any general type
of device in a circuit doing something useful with electrical energy. Any non-specific electrical
device is generally called a load, so if you see a schematic diagram showing a resistor symbol
labeled "load," especially in a tutorial circuit diagram explaining some concept unrelated to the
actual use of electrical power, that symbol may just be a kind of shorthand representation of
something else more practical than a resistor.
To summarize what we've learned in this lesson, let's analyze the following circuit, deter-
mining all that we can from the information given:
Battery
All we've been given here to start with is the battery voltage (10 volts) and the circuit
2.6.
NONLINEAR CONDUCTION
51
current (2 amps). We don't know the resistor's resistance in ohms or the power dissipated by
it in watts. Surveying our array of Ohm's Law equations, we find two equations that give us
answers from known quantities of voltage and current:
and
E
I
Inserting the known quantities of voltage (E) and current (I) into these two equations, we
can determine circuit resistance (R) and power dissipation (P):
10 V
2 A
5 Ω
(2 A)(10 V) = 20 W
For the circuit conditions of 10 volts and 2 amps, the resistor's resistance must be 5 Ω. If
we were designing a circuit to operate at these values, we would have to specify a resistor with
a minimum power rating of 20 watts, or else it would overheat and fail.
• REVIEW:
• Devices called resistors are built to provide precise amounts of resistance in electric cir-
cuits. Resistors are rated both in terms of their resistance (ohms) and their ability to
dissipate heat energy (watts).
• Resistor resistance ratings cannot be determined from the physical size of the resistor(s)
in question, although approximate power ratings can. The larger the resistor is, the more
power it can safely dissipate without suffering damage.
• Any device that performs some useful task with electric power is generally known as a
load. Sometimes resistor symbols are used in schematic diagrams to designate a non-
specific load, rather than an actual resistor.
2.6
Nonlinear conduction
"Advances are made by answering questions. Discoveries are made by questioning
answers."
Bernhard Haisch, Astrophysicist
Ohm's Law is a simple and powerful mathematical tool for helping us analyze electric cir-
cuits, but it has limitations, and we must understand these limitations in order to properly
apply it to real circuits. For most conductors, resistance is a rather stable property, largely
unaffected by voltage or current. For this reason we can regard the resistance of many circuit
components as a constant, with voltage and current being directly related to each other.
For instance, our previous circuit example with the 3 Ωlamp, we calculated current through
the circuit by dividing voltage by resistance (I=E/R). With an 18 volt battery, our circuit current
was 6 amps. Doubling the battery voltage to 36 volts resulted in a doubled current of 12 amps.
52
CHAPTER 2. OHM'S LAW
All of this makes sense, of course, so long as the lamp continues to provide exactly the same
amount of friction (resistance) to the flow of electrons through it: 3 Ω.
Battery
Lamp
Battery
Lamp
18 V
36 V
However, reality is not always this simple. One of the phenomena explored in a later chap-
ter is that of conductor resistance changing with temperature. In an incandescent lamp (the
kind employing the principle of electric current heating a thin filament of wire to the point that
it glows white-hot), the resistance of the filament wire will increase dramatically as it warms
from room temperature to operating temperature. If we were to increase the supply voltage
in a real lamp circuit, the resulting increase in current would cause the filament to increase
temperature, which would in turn increase its resistance, thus preventing further increases in
current without further increases in battery voltage. Consequently, voltage and current do not
follow the simple equation "I=E/R" (with R assumed to be equal to 3 Ω) because an incandescent
lamp's filament resistance does not remain stable for different currents.
The phenomenon of resistance changing with variations in temperature is one shared by
almost all metals, of which most wires are made. For most applications, these changes in
resistance are small enough to be ignored. In the application of metal lamp filaments, the
change happens to be quite large.
This is just one example of "nonlinearity" in electric circuits. It is by no means the only
example. A "linear" function in mathematics is one that tracks a straight line when plotted on
a graph. The simplified version of the lamp circuit with a constant filament resistance of 3 Ω
generates a plot like this:
2.6.
NONLINEAR CONDUCTION
53
I
(current)
E
(voltage)
The straight-line plot of current over voltage indicates that resistance is a stable, unchang-
ing value for a wide range of circuit voltages and currents. In an "ideal" situation, this is the
case. Resistors, which are manufactured to provide a definite, stable value of resistance, be-
have very much like the plot of values seen above. A mathematician would call their behavior
"linear."
A more realistic analysis of a lamp circuit, however, over several different values of battery
voltage would generate a plot of this shape:
I
(current)
E
(voltage)
The plot is no longer a straight line. It rises sharply on the left, as voltage increases from
zero to a low level. As it progresses to the right we see the line flattening out, the circuit
requiring greater and greater increases in voltage to achieve equal increases in current.
If we try to apply Ohm's Law to find the resistance of this lamp circuit with the voltage
and current values plotted above, we arrive at several different values. We could say that the
resistance here is nonlinear, increasing with increasing current and voltage. The nonlinearity
is caused by the effects of high temperature on the metal wire of the lamp filament.
Another example of nonlinear current conduction is through gases such as air. At standard
temperatures and pressures, air is an effective insulator. However, if the voltage between two
conductors separated by an air gap is increased greatly enough, the air molecules between the
54
CHAPTER 2. OHM'S LAW
gap will become "ionized," having their electrons stripped off by the force of the high voltage
between the wires. Once ionized, air (and other gases) become good conductors of electricity,
allowing electron flow where none could exist prior to ionization. If we were to plot current over
voltage on a graph as we did with the lamp circuit, the effect of ionization would be clearly seen
as nonlinear:
I
(current)
E
(voltage)
ionization potential
0
50
100
150
200
250
300
350
400
The graph shown is approximate for a small air gap (less than one inch). A larger air gap
would yield a higher ionization potential, but the shape of the I/E curve would be very similar:
practically no current until the ionization potential was reached, then substantial conduction
after that.
Incidentally, this is the reason lightning bolts exist as momentary surges rather than con-
tinuous flows of electrons. The voltage built up between the earth and clouds (or between
different sets of clouds) must increase to the point where it overcomes the ionization potential
of the air gap before the air ionizes enough to support a substantial flow of electrons. Once it
does, the current will continue to conduct through the ionized air until the static charge be-
tween the two points depletes. Once the charge depletes enough so that the voltage falls below
another threshold point, the air de-ionizes and returns to its normal state of extremely high
resistance.
Many solid insulating materials exhibit similar resistance properties: extremely high re-
sistance to electron flow below some critical threshold voltage, then a much lower resistance
at voltages beyond that threshold. Once a solid insulating material has been compromised by
high-voltage breakdown, as it is called, it often does not return to its former insulating state,
unlike most gases. It may insulate once again at low voltages, but its breakdown threshold
voltage will have been decreased to some lower level, which may allow breakdown to occur
more easily in the future. This is a common mode of failure in high-voltage wiring: insulation
damage due to breakdown. Such failures may be detected through the use of special resistance
meters employing high voltage (1000 volts or more).
There are circuit components specifically engineered to provide nonlinear resistance curves,
one of them being the varistor. Commonly manufactured from compounds such as zinc oxide
or silicon carbide, these devices maintain high resistance across their terminals until a cer-
2.6.
NONLINEAR CONDUCTION
55
tain "firing" or "breakdown" voltage (equivalent to the "ionization potential" of an air gap)
is reached, at which point their resistance decreases dramatically. Unlike the breakdown of
an insulator, varistor breakdown is repeatable: that is, it is designed to withstand repeated
breakdowns without failure. A picture of a varistor is shown here:
There are also special gas-filled tubes designed to do much the same thing, exploiting the
very same principle at work in the ionization of air by a lightning bolt.
Other electrical components exhibit even stranger current/voltage curves than this. Some
devices actually experience a decrease in current as the applied voltage increases. Because the
slope of the current/voltage for this phenomenon is negative (angling down instead of up as it
progresses from left to right), it is known as negative resistance.
I
(current)
E
(voltage)
negative
resistance
region of
Most notably, high-vacuum electron tubes known as tetrodes and semiconductor diodes
known as Esaki or tunnel diodes exhibit negative resistance for certain ranges of applied volt-
56
CHAPTER 2. OHM'S LAW
age.
Ohm's Law is not very useful for analyzing the behavior of components like these where re-
sistance varies with voltage and current. Some have even suggested that "Ohm's Law" should
be demoted from the status of a "Law" because it is not universal. It might be more accurate
to call the equation (R=E/I) a definition of resistance, befitting of a certain class of materials
under a narrow range of conditions.
For the benefit of the student, however, we will assume that resistances specified in example
circuits are stable over a wide range of conditions unless otherwise specified. I just wanted to
expose you to a little bit of the complexity of the real world, lest I give you the false impression
that the whole of electrical phenomena could be summarized in a few simple equations.
• REVIEW:
• The resistance of most conductive materials is stable over a wide range of conditions, but
this is not true of all materials.
• Any function that can be plotted on a graph as a straight line is called a linear function.
For circuits with stable resistances, the plot of current over voltage is linear (I=E/R).
• In circuits where resistance varies with changes in either voltage or current, the plot of
current over voltage will be nonlinear (not a straight line).
• A varistor is a component that changes resistance with the amount of voltage impressed
across it. With little voltage across it, its resistance is high. Then, at a certain "break-
down" or "firing" voltage, its resistance decreases dramatically.
• Negative resistance is where the current through a component actually decreases as the
applied voltage across it is increased. Some electron tubes and semiconductor diodes
(most notably, the tetrode tube and the Esaki, or tunnel diode, respectively) exhibit nega-
tive resistance over a certain range of voltages.
2.7
Circuit wiring
So far, we've been analyzing single-battery, single-resistor circuits with no regard for the con-
necting wires between the components, so long as a complete circuit is formed. Does the wire
length or circuit "shape" matter to our calculations? Let's look at a couple of circuit configura-
tions and find out:
2.7.
CIRCUIT WIRING
57
Battery
Resistor
1
2
3
4
Battery
Resistor
2
3
4
1
10 V
10 V
5 Ω
5 Ω
When we draw wires connecting points in a circuit, we usually assume those wires have
negligible resistance. As such, they contribute no appreciable effect to the overall resistance
of the circuit, and so the only resistance we have to contend with is the resistance in the
components. In the above circuits, the only resistance comes from the 5 Ωresistors, so that is
all we will consider in our calculations. In real life, metal wires actually do have resistance (and
so do power sources!), but those resistances are generally so much smaller than the resistance
present in the other circuit components that they can be safely ignored. Exceptions to this
rule exist in power system wiring, where even very small amounts of conductor resistance can
create significant voltage drops given normal (high) levels of current.
If connecting wire resistance is very little or none, we can regard the connected points in
a circuit as being electrically common. That is, points 1 and 2 in the above circuits may be
physically joined close together or far apart, and it doesn't matter for any voltage or resistance
measurements relative to those points. The same goes for points 3 and 4. It is as if the ends
of the resistor were attached directly across the terminals of the battery, so far as our Ohm's
Law calculations and voltage measurements are concerned. This is useful to know, because it
means you can re-draw a circuit diagram or re-wire a circuit, shortening or lengthening the
wires as desired without appreciably impacting the circuit's function. All that matters is that
the components attach to each other in the same sequence.
It also means that voltage measurements between sets of "electrically common" points will
be the same. That is, the voltage between points 1 and 6 (directly across the battery) will be
the same as the voltage between points 3 and 4 (directly across the resistor). Take a close look
at the following circuit, and try to determine which points are common to each other:
58
CHAPTER 2. OHM'S LAW
Battery
Resistor
1
2
3
4
5
6
10 V
5 Ω
Here, we only have 2 components excluding the wires: the battery and the resistor. Though
the connecting wires take a convoluted path in forming a complete circuit, there are several
electrically common points in the electrons' path. Points 1, 2, and 3 are all common to each
other, because they're directly connected together by wire. The same goes for points 4, 5, and
6.
The voltage between points 1 and 6 is 10 volts, coming straight from the battery. However,
since points 5 and 4 are common to 6, and points 2 and 3 common to 1, that same 10 volts also
exists between these other pairs of points:
Between points 1 and 4 = 10 volts
Between points 2 and 4 = 10 volts
Between points 3 and 4 = 10 volts (directly across the resistor)
Between points 1 and 5 = 10 volts
Between points 2 and 5 = 10 volts
Between points 3 and 5 = 10 volts
Between points 1 and 6 = 10 volts (directly across the battery)
Between points 2 and 6 = 10 volts
Between points 3 and 6 = 10 volts
Since electrically common points are connected together by (zero resistance) wire, there
is no significant voltage drop between them regardless of the amount of current conducted
from one to the next through that connecting wire. Thus, if we were to read voltages between
common points, we should show (practically) zero:
Between points 1 and 2 = 0 volts
Points 1, 2, and 3 are
Between points 2 and 3 = 0 volts
electrically common
Between points 1 and 3 = 0 volts
Between points 4 and 5 = 0 volts
Points 4, 5, and 6 are
Between points 5 and 6 = 0 volts
electrically common
Between points 4 and 6 = 0 volts
This makes sense mathematically, too. With a 10 volt battery and a 5 Ωresistor, the circuit
current will be 2 amps. With wire resistance being zero, the voltage drop across any continuous
stretch of wire can be determined through Ohm's Law as such:
2.7.
CIRCUIT WIRING
59
It should be obvious that the calculated voltage drop across any uninterrupted length of
wire in a circuit where wire is assumed to have zero resistance will always be zero, no matter
what the magnitude of current, since zero multiplied by anything equals zero.
Because common points in a circuit will exhibit the same relative voltage and resistance
measurements, wires connecting common points are often labeled with the same designation.
This is not to say that the terminal connection points are labeled the same, just the connecting
wires. Take this circuit as an example:
Battery
Resistor
1
2
3
4
5
6
10 V
5 Ω
Points 1, 2, and 3 are all common to each other, so the wire connecting point 1 to 2 is
labeled the same (wire 2) as the wire connecting point 2 to 3 (wire 2). In a real circuit, the
wire stretching from point 1 to 2 may not even be the same color or size as the wire connecting
point 2 to 3, but they should bear the exact same label. The same goes for the wires connecting
points 6, 5, and 4.
Knowing that electrically common points have zero voltage drop between them is a valuable
troubleshooting principle. If I measure for voltage between points in a circuit that are supposed
to be common to each other, I should read zero. If, however, I read substantial voltage between
those two points, then I know with certainty that they cannot be directly connected together.
If those points are supposed to be electrically common but they register otherwise, then I know
that there is an "open failure" between those points.
One final note: for most practical purposes, wire conductors can be assumed to possess zero
resistance from end to end. In reality, however, there will always be some small amount of
resistance encountered along the length of a wire, unless its a superconducting wire. Knowing
this, we need to bear in mind that the principles learned here about electrically common points
are all valid to a large degree, but not to an absolute degree. That is, the rule that electrically
common points are guaranteed to have zero voltage between them is more accurately stated
as such: electrically common points will have very little voltage dropped between them. That
small, virtually unavoidable trace of resistance found in any piece of connecting wire is bound
60
CHAPTER 2. OHM'S LAW
to create a small voltage across the length of it as current is conducted through. So long as you
understand that these rules are based upon ideal conditions, you won't be perplexed when you
come across some condition appearing to be an exception to the rule.
• REVIEW:
• Connecting wires in a circuit are assumed to have zero resistance unless otherwise stated.
• Wires in a circuit can be shortened or lengthened without impacting the circuit's func-
tion - all that matters is that the components are attached to one another in the same
sequence.
• Points directly connected together in a circuit by zero resistance (wire) are considered to
be electrically common.
• Electrically common points, with zero resistance between them, will have zero voltage
dropped between them, regardless of the magnitude of current (ideally).
• The voltage or resistance readings referenced between sets of electrically common points
will be the same.
• These rules apply to ideal conditions, where connecting wires are assumed to possess ab-
solutely zero resistance. In real life this will probably not be the case, but wire resistances
should be low enough so that the general principles stated here still hold.
2.8
Polarity of voltage drops
We can trace the direction that electrons will flow in the same circuit by starting at the negative
(-) terminal and following through to the positive (+) terminal of the battery, the only source of
voltage in the circuit. From this we can see that the electrons are moving counter-clockwise,
from point 6 to 5 to 4 to 3 to 2 to 1 and back to 6 again.
As the current encounters the 5 Ωresistance, voltage is dropped across the resistor's ends.
The polarity of this voltage drop is negative (-) at point 4 with respect to positive (+) at point
3. We can mark the polarity of the resistor's voltage drop with these negative and positive
symbols, in accordance with the direction of current (whichever end of the resistor the current
is entering is negative with respect to the end of the resistor it is exiting:
Battery
Resistor
1
2
3
4
5
6
current
current
10 V
5 Ω
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
61
We could make our table of voltages a little more complete by marking the polarity of the
voltage for each pair of points in this circuit:
Between points 1 (+) and 4 (-) = 10 volts
Between points 2 (+) and 4 (-) = 10 volts
Between points 3 (+) and 4 (-) = 10 volts
Between points 1 (+) and 5 (-) = 10 volts
Between points 2 (+) and 5 (-) = 10 volts
Between points 3 (+) and 5 (-) = 10 volts
Between points 1 (+) and 6 (-) = 10 volts
Between points 2 (+) and 6 (-) = 10 volts
Between points 3 (+) and 6 (-) = 10 volts
While it might seem a little silly to document polarity of voltage drop in this circuit, it is
an important concept to master. It will be critically important in the analysis of more complex
circuits involving multiple resistors and/or batteries.
It should be understood that polarity has nothing to do with Ohm's Law: there will never
be negative voltages, currents, or resistance entered into any Ohm's Law equations! There are
other mathematical principles of electricity that do take polarity into account through the use
of signs (+ or -), but not Ohm's Law.
• REVIEW:
• The polarity of the voltage drop across any resistive component is determined by the
direction of electron flow through it: negative entering, and positive exiting.
2.9
Computer simulation of electric circuits
Computers can be powerful tools if used properly, especially in the realms of science and engi-
neering. Software exists for the simulation of electric circuits by computer, and these programs
can be very useful in helping circuit designers test ideas before actually building real circuits,
saving much time and money.
These same programs can be fantastic aids to the beginning student of electronics, allowing
the exploration of ideas quickly and easily with no assembly of real circuits required. Of course,
there is no substitute for actually building and testing real circuits, but computer simulations
certainly assist in the learning process by allowing the student to experiment with changes
and see the effects they have on circuits. Throughout this book, I'll be incorporating computer
printouts from circuit simulation frequently in order to illustrate important concepts. By ob-
serving the results of a computer simulation, a student can gain an intuitive grasp of circuit
behavior without the intimidation of abstract mathematical analysis.
To simulate circuits on computer, I make use of a particular program called SPICE, which
works by describing a circuit to the computer by means of a listing of text. In essence, this
listing is a kind of computer program in itself, and must adhere to the syntactical rules of
the SPICE language. The computer is then used to process, or "run," the SPICE program,
which interprets the text listing describing the circuit and outputs the results of its detailed
mathematical analysis, also in text form. Many details of using SPICE are described in volume
62
CHAPTER 2. OHM'S LAW
5 ("Reference") of this book series for those wanting more information. Here, I'll just introduce
the basic concepts and then apply SPICE to the analysis of these simple circuits we've been
reading about.
First, we need to have SPICE installed on our computer. As a free program, it is commonly
available on the internet for download, and in formats appropriate for many different operating
systems. In this book, I use one of the earlier versions of SPICE: version 2G6, for its simplicity
of use.
Next, we need a circuit for SPICE to analyze. Let's try one of the circuits illustrated earlier
in the chapter. Here is its schematic diagram:
Battery
10 V
5 Ω
R1
This simple circuit consists of a battery and a resistor connected directly together. We know
the voltage of the battery (10 volts) and the resistance of the resistor (5 Ω), but nothing else
about the circuit. If we describe this circuit to SPICE, it should be able to tell us (at the very
least), how much current we have in the circuit by using Ohm's Law (I=E/R).
SPICE cannot directly understand a schematic diagram or any other form of graphical
description. SPICE is a text-based computer program, and demands that a circuit be described
in terms of its constituent components and connection points. Each unique connection point
in a circuit is described for SPICE by a "node" number. Points that are electrically common to
each other in the circuit to be simulated are designated as such by sharing the same number.
It might be helpful to think of these numbers as "wire" numbers rather than "node" numbers,
following the definition given in the previous section. This is how the computer knows what's
connected to what: by the sharing of common wire, or node, numbers. In our example circuit,
we only have two "nodes," the top wire and the bottom wire. SPICE demands there be a node
0 somewhere in the circuit, so we'll label our wires 0 and 1:
Battery
1
0
1
0
10 V
R1
5 Ω
1
1
0
0
0
0
1
1
In the above illustration, I've shown multiple "1" and "0" labels around each respective wire
to emphasize the concept of common points sharing common node numbers, but still this is a
graphic image, not a text description. SPICE needs to have the component values and node
numbers given to it in text form before any analysis may proceed.
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
63
Creating a text file in a computer involves the use of a program called a text editor. Similar
to a word processor, a text editor allows you to type text and record what you've typed in the
form of a file stored on the computer's hard disk. Text editors lack the formatting ability of
word processors (no italic, bold, or underlined characters), and this is a good thing, since
programs such as SPICE wouldn't know what to do with this extra information. If we want
to create a plain-text file, with absolutely nothing recorded except the keyboard characters we
select, a text editor is the tool to use.
If using a Microsoft operating system such as DOS or Windows, a couple of text editors are
readily available with the system. In DOS, there is the old Edit text editing program, which
may be invoked by typing edit at the command prompt. In Windows (3.x/95/98/NT/Me/2k/XP),
the Notepad text editor is your stock choice. Many other text editing programs are available,
and some are even free. I happen to use a free text editor called Vim, and run it under both
Windows 95 and Linux operating systems. It matters little which editor you use, so don't worry
if the screenshots in this section don't look like yours; the important information here is what
you type, not which editor you happen to use.
To describe this simple, two-component circuit to SPICE, I will begin by invoking my text
editor program and typing in a "title" line for the circuit:
We can describe the battery to the computer by typing in a line of text starting with the
letter "v" (for "Voltage source"), identifying which wire each terminal of the battery connects
to (the node numbers), and the battery's voltage, like this:
64
CHAPTER 2. OHM'S LAW
This line of text tells SPICE that we have a voltage source connected between nodes 1 and
0, direct current (DC), 10 volts. That's all the computer needs to know regarding the battery.
Now we turn to the resistor: SPICE requires that resistors be described with a letter "r," the
numbers of the two nodes (connection points), and the resistance in ohms. Since this is a
computer simulation, there is no need to specify a power rating for the resistor. That's one nice
thing about "virtual" components: they can't be harmed by excessive voltages or currents!
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
65
Now, SPICE will know there is a resistor connected between nodes 1 and 0 with a value of
5 Ω. This very brief line of text tells the computer we have a resistor ("r") connected between
the same two nodes as the battery (1 and 0), with a resistance value of 5 Ω.
If we add an .end statement to this collection of SPICE commands to indicate the end of
the circuit description, we will have all the information SPICE needs, collected in one file and
ready for processing. This circuit description, comprised of lines of text in a computer file, is
technically known as a netlist, or deck:
66
CHAPTER 2. OHM'S LAW
Once we have finished typing all the necessary SPICE commands, we need to "save" them to
a file on the computer's hard disk so that SPICE has something to reference to when invoked.
Since this is my first SPICE netlist, I'll save it under the filename "circuit1.cir" (the actual
name being arbitrary). You may elect to name your first SPICE netlist something completely
different, just as long as you don't violate any filename rules for your operating system, such
as using no more than 8+3 characters (eight characters in the name, and three characters in
the extension: 12345678.123) in DOS.
To invoke SPICE (tell it to process the contents of the circuit1.cir netlist file), we have
to exit from the text editor and access a command prompt (the "DOS prompt" for Microsoft
users) where we can enter text commands for the computer's operating system to obey. This
"primitive" way of invoking a program may seem archaic to computer users accustomed to a
"point-and-click" graphical environment, but it is a very powerful and flexible way of doing
things. Remember, what you're doing here by using SPICE is a simple form of computer pro-
gramming, and the more comfortable you become in giving the computer text-form commands
to follow - as opposed to simply clicking on icon images using a mouse - the more mastery you
will have over your computer.
Once at a command prompt, type in this command, followed by an [Enter] keystroke (this
example uses the filename circuit1.cir; if you have chosen a different filename for your
netlist file, substitute it):
spice < circuit1.cir
Here is how this looks on my computer (running the Linux operating system), just before I
press the [Enter] key:
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
67
As soon as you press the [Enter] key to issue this command, text from SPICE's output
should scroll by on the computer screen. Here is a screenshot showing what SPICE outputs
on my computer (I've lengthened the "terminal" window to show you the full text. With a
normal-size terminal, the text easily exceeds one page length):
68
CHAPTER 2. OHM'S LAW
SPICE begins with a reiteration of the netlist, complete with title line and .end statement.
About halfway through the simulation it displays the voltage at all nodes with reference to
node 0. In this example, we only have one node other than node 0, so it displays the voltage
there: 10.0000 volts. Then it displays the current through each voltage source. Since we only
have one voltage source in the entire circuit, it only displays the current through that one. In
this case, the source current is 2 amps. Due to a quirk in the way SPICE analyzes current, the
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
69
value of 2 amps is output as a negative (-) 2 amps.
The last line of text in the computer's analysis report is "total power dissipation," which in
this case is given as "2.00E+01" watts: 2.00 x 101, or 20 watts. SPICE outputs most figures
in scientific notation rather than normal (fixed-point) notation. While this may seem to be
more confusing at first, it is actually less confusing when very large or very small numbers are
involved. The details of scientific notation will be covered in the next chapter of this book.
One of the benefits of using a "primitive" text-based program such as SPICE is that the text
files dealt with are extremely small compared to other file formats, especially graphical formats
used in other circuit simulation software. Also, the fact that SPICE's output is plain text means
you can direct SPICE's output to another text file where it may be further manipulated. To do
this, we re-issue a command to the computer's operating system to invoke SPICE, this time
redirecting the output to a file I'll call "output.txt":
SPICE will run "silently" this time, without the stream of text output to the computer
screen as before. A new file, output1.txt, will be created, which you may open and change
using a text editor or word processor. For this illustration, I'll use the same text editor (Vim)
to open this file:
70
CHAPTER 2. OHM'S LAW
Now, I may freely edit this file, deleting any extraneous text (such as the "banners" showing
date and time), leaving only the text that I feel to be pertinent to my circuit's analysis:
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
71
Once suitably edited and re-saved under the same filename (output.txt in this example),
the text may be pasted into any kind of document, "plain text" being a universal file format
for almost all computer systems. I can even include it directly in the text of this book - rather
than as a "screenshot" graphic image - like this:
my first circuit
v 1 0 dc 10
r 1 0 5
.end
node
voltage
1)
10.0000
voltage source currents
name
current
v
total power dissipation
watts
Incidentally, this is the preferred format for text output from SPICE simulations in this
book series: as real text, not as graphic screenshot images.
To alter a component value in the simulation, we need to open up the netlist file (circuit1.cir)
and make the required modifications in the text description of the circuit, then save those
changes to the same filename, and re-invoke SPICE at the command prompt. This process of
72
CHAPTER 2. OHM'S LAW
editing and processing a text file is one familiar to every computer programmer. One of the
reasons I like to teach SPICE is that it prepares the learner to think and work like a computer
programmer, which is good because computer programming is a significant area of advanced
electronics work.
Earlier we explored the consequences of changing one of the three variables in an electric
circuit (voltage, current, or resistance) using Ohm's Law to mathematically predict what would
happen. Now let's try the same thing using SPICE to do the math for us.
If we were to triple the voltage in our last example circuit from 10 to 30 volts and keep the
circuit resistance unchanged, we would expect the current to triple as well. Let's try this, re-
naming our netlist file so as to not over-write the first file. This way, we will have both versions
of the circuit simulation stored on the hard drive of our computer for future use. The following
text listing is the output of SPICE for this modified netlist, formatted as plain text rather than
as a graphic image of my computer screen:
second example circuit
v 1 0 dc 30
r 1 0 5
.end
node
voltage
1)
30.0000
voltage source currents
name
current
v
total power dissipation
watts
Just as we expected, the current tripled with the voltage increase. Current used to be
2 amps, but now it has increased to 6 amps (-6.000 x 100). Note also how the total power
dissipation in the circuit has increased. It was 20 watts before, but now is 180 watts (1.8 x
102). Recalling that power is related to the square of the voltage (Joule's Law: P=E2/R), this
makes sense. If we triple the circuit voltage, the power should increase by a factor of nine (32
= 9). Nine times 20 is indeed 180, so SPICE's output does indeed correlate with what we know
about power in electric circuits.
If we want to see how this simple circuit would respond over a wide range of battery volt-
ages, we can invoke some of the more advanced options within SPICE. Here, I'll use the ".dc"
analysis option to vary the battery voltage from 0 to 100 volts in 5 volt increments, printing
out the circuit voltage and current at every step. The lines in the SPICE netlist beginning with
a star symbol ("*") are comments. That is, they don't tell the computer to do anything relating
to circuit analysis, but merely serve as notes for any human being reading the netlist text.
third example circuit
v 1 0
r 1 0 5
*the ".dc" statement tells spice to sweep the "v" supply
*voltage from 0 to 100 volts in 5 volt steps.
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
73
.dc v 0 100 5
.print dc v(1) i(v)
.end
74
CHAPTER 2. OHM'S LAW
The .print command in this SPICE netlist instructs SPICE to print columns of numbers
corresponding to each step in the analysis:
v
i(v)
0.000E+00
0.000E+00
5.000E+00
1.000E+01
1.500E+01
2.000E+01
2.500E+01
3.000E+01
3.500E+01
4.000E+01
4.500E+01
5.000E+01
5.500E+01
6.000E+01
6.500E+01
7.000E+01
7.500E+01
8.000E+01
8.500E+01
9.000E+01
9.500E+01
1.000E+02
2.9.
COMPUTER SIMULATION OF ELECTRIC CIRCUITS
75
If I re-edit the netlist file, changing the .print command into a .plot command, SPICE
will output a crude graph made up of text characters:
Legend:
sweep
0.000e+00
0.000e+00 .
sweep
In both output formats, the left-hand column of numbers represents the battery voltage at
each interval, as it increases from 0 volts to 100 volts, 5 volts at a time. The numbers in the
right-hand column indicate the circuit current for each of those voltages. Look closely at those
numbers and you'll see the proportional relationship between each pair: Ohm's Law (I=E/R)
holds true in each and every case, each current value being 1/5 the respective voltage value,
because the circuit resistance is exactly 5 Ω. Again, the negative numbers for current in this
SPICE analysis is more of a quirk than anything else. Just pay attention to the absolute value
of each number unless otherwise specified.
There are even some computer programs able to interpret and convert the non-graphical
data output by SPICE into a graphical plot. One of these programs is called Nutmeg, and its
output looks something like this:
76
CHAPTER 2. OHM'S LAW
Note how Nutmeg plots the resistor voltage v(1) (voltage between node 1 and the implied
reference point of node 0) as a line with a positive slope (from lower-left to upper-right).
Whether or not you ever become proficient at using SPICE is not relevant to its application
in this book. All that matters is that you develop an understanding for what the numbers
mean in a SPICE-generated report. In the examples to come, I'll do my best to annotate the
numerical results of SPICE to eliminate any confusion, and unlock the power of this amazing
tool to help you understand the behavior of electric circuits.
2.10
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Larry Cramblett (September 20, 2004): identified serious typographical error in "Nonlin-
ear conduction" section.
James Boorn (January 18, 2001): identified sentence structure error and offered correc-
tion. Also, identified discrepancy in netlist syntax requirements between SPICE version 2g6
and version 3f5.
Ben Crowell, Ph.D. (January 13, 2001): suggestions on improving the technical accuracy
of voltage and charge definitions.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 3
ELECTRICAL SAFETY
Contents
3.1
The importance of electrical safety
. . . . . . . . . . . . . . . . . . . . . . . 77
3.2
Physiological effects of electricity . . . . . . . . . . . . . . . . . . . . . . . . 78
3.3
Shock current path . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 80
3.4
Ohm's Law (again!) . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 86
3.5
Safe practices . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 93
3.6
Emergency response . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 96
3.7
Common sources of hazard . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 98
3.8
Safe circuit design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 100
3.9
Safe meter usage . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 106
3.10 Electric shock data . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 116
3.11 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 117
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 117
3.1
The importance of electrical safety
With this lesson, I hope to avoid a common mistake found in electronics textbooks of either
ignoring or not covering with sufficient detail the subject of electrical safety. I assume that
whoever reads this book has at least a passing interest in actually working with electricity, and
as such the topic of safety is of paramount importance. Those authors, editors, and publishers
who fail to incorporate this subject into their introductory texts are depriving the reader of
life-saving information.
As an instructor of industrial electronics, I spend a full week with my students reviewing
the theoretical and practical aspects of electrical safety. The same textbooks I found lacking
in technical clarity I also found lacking in coverage of electrical safety, hence the creation of
this chapter. Its placement after the first two chapters is intentional: in order for the con-
cepts of electrical safety to make the most sense, some foundational knowledge of electricity is
necessary.
77
78
CHAPTER 3. ELECTRICAL SAFETY
Another benefit of including a detailed lesson on electrical safety is the practical context it
sets for basic concepts of voltage, current, resistance, and circuit design. The more relevant a
technical topic can be made, the more likely a student will be to pay attention and comprehend.
And what could be more relevant than application to your own personal safety? Also, with
electrical power being such an everyday presence in modern life, almost anyone can relate to
the illustrations given in such a lesson. Have you ever wondered why birds don't get shocked
while resting on power lines? Read on and find out!
3.2
Physiological effects of electricity
Most of us have experienced some form of electric "shock," where electricity causes our body
to experience pain or trauma. If we are fortunate, the extent of that experience is limited to
tingles or jolts of pain from static electricity buildup discharging through our bodies. When we
are working around electric circuits capable of delivering high power to loads, electric shock
becomes a much more serious issue, and pain is the least significant result of shock.
As electric current is conducted through a material, any opposition to that flow of electrons
(resistance) results in a dissipation of energy, usually in the form of heat. This is the most basic
and easy-to-understand effect of electricity on living tissue: current makes it heat up. If the
amount of heat generated is sufficient, the tissue may be burnt. The effect is physiologically
the same as damage caused by an open flame or other high-temperature source of heat, except
that electricity has the ability to burn tissue well beneath the skin of a victim, even burning
internal organs.
Another effect of electric current on the body, perhaps the most significant in terms of haz-
ard, regards the nervous system. By "nervous system" I mean the network of special cells in
the body called "nerve cells" or "neurons" which process and conduct the multitude of signals
responsible for regulation of many body functions. The brain, spinal cord, and sensory/motor
organs in the body function together to allow it to sense, move, respond, think, and remember.
Nerve cells communicate to each other by acting as "transducers:" creating electrical sig-
nals (very small voltages and currents) in response to the input of certain chemical compounds
called neurotransmitters, and releasing neurotransmitters when stimulated by electrical sig-
nals. If electric current of sufficient magnitude is conducted through a living creature (human
or otherwise), its effect will be to override the tiny electrical impulses normally generated by
the neurons, overloading the nervous system and preventing both reflex and volitional sig-
nals from being able to actuate muscles. Muscles triggered by an external (shock) current will
involuntarily contract, and there's nothing the victim can do about it.
This problem is especially dangerous if the victim contacts an energized conductor with his
or her hands. The forearm muscles responsible for bending fingers tend to be better developed
than those muscles responsible for extending fingers, and so if both sets of muscles try to con-
tract because of an electric current conducted through the person's arm, the "bending" muscles
will win, clenching the fingers into a fist. If the conductor delivering current to the victim faces
the palm of his or her hand, this clenching action will force the hand to grasp the wire firmly,
thus worsening the situation by securing excellent contact with the wire. The victim will be
completely unable to let go of the wire.
Medically, this condition of involuntary muscle contraction is called tetanus. Electricians
familiar with this effect of electric shock often refer to an immobilized victim of electric shock
3.2.
PHYSIOLOGICAL EFFECTS OF ELECTRICITY
79
as being "froze on the circuit." Shock-induced tetanus can only be interrupted by stopping the
current through the victim.
Even when the current is stopped, the victim may not regain voluntary control over their
muscles for a while, as the neurotransmitter chemistry has been thrown into disarray. This
principle has been applied in "stun gun" devices such as Tasers, which on the principle of
momentarily shocking a victim with a high-voltage pulse delivered between two electrodes. A
well-placed shock has the effect of temporarily (a few minutes) immobilizing the victim.
Electric current is able to affect more than just skeletal muscles in a shock victim, how-
ever. The diaphragm muscle controlling the lungs, and the heart - which is a muscle in itself
- can also be "frozen" in a state of tetanus by electric current. Even currents too low to in-
duce tetanus are often able to scramble nerve cell signals enough that the heart cannot beat
properly, sending the heart into a condition known as fibrillation. A fibrillating heart flutters
rather than beats, and is ineffective at pumping blood to vital organs in the body. In any case,
death from asphyxiation and/or cardiac arrest will surely result from a strong enough electric
current through the body. Ironically, medical personnel use a strong jolt of electric current
applied across the chest of a victim to "jump start" a fibrillating heart into a normal beating
pattern.
That last detail leads us into another hazard of electric shock, this one peculiar to public
power systems. Though our initial study of electric circuits will focus almost exclusively on DC
(Direct Current, or electricity that moves in a continuous direction in a circuit), modern power
systems utilize alternating current, or AC. The technical reasons for this preference of AC over
DC in power systems are irrelevant to this discussion, but the special hazards of each kind of
electrical power are very important to the topic of safety.
How AC affects the body depends largely on frequency. Low-frequency (50- to
60-Hz) AC is used in US (60 Hz) and European (50 Hz) households; it can be more
dangerous than high-frequency AC and is 3 to 5 times more dangerous than DC
of the same voltage and amperage. Low-frequency AC produces extended muscle
contraction (tetany), which may freeze the hand to the current's source, prolonging
exposure. DC is most likely to cause a single convulsive contraction, which often
forces the victim away from the current's source. [1]
AC's alternating nature has a greater tendency to throw the heart's pacemaker neurons into
a condition of fibrillation, whereas DC tends to just make the heart stand still. Once the shock
current is halted, a "frozen" heart has a better chance of regaining a normal beat pattern than
a fibrillating heart. This is why "defibrillating" equipment used by emergency medics works:
the jolt of current supplied by the defibrillator unit is DC, which halts fibrillation and gives the
heart a chance to recover.
In either case, electric currents high enough to cause involuntary muscle action are dan-
gerous and are to be avoided at all costs. In the next section, we'll take a look at how such
currents typically enter and exit the body, and examine precautions against such occurrences.
• REVIEW:
• Electric current is capable of producing deep and severe burns in the body due to power
dissipation across the body's electrical resistance.
80
CHAPTER 3. ELECTRICAL SAFETY
• Tetanus is the condition where muscles involuntarily contract due to the passage of ex-
ternal electric current through the body. When involuntary contraction of muscles con-
trolling the fingers causes a victim to be unable to let go of an energized conductor, the
victim is said to be "froze on the circuit."
• Diaphragm (lung) and heart muscles are similarly affected by electric current.
Even
currents too small to induce tetanus can be strong enough to interfere with the heart's
pacemaker neurons, causing the heart to flutter instead of strongly beat.
• Direct current (DC) is more likely to cause muscle tetanus than alternating current (AC),
making DC more likely to "freeze" a victim in a shock scenario. However, AC is more
likely to cause a victim's heart to fibrillate, which is a more dangerous condition for the
victim after the shocking current has been halted.
3.3
Shock current path
As we've already learned, electricity requires a complete path (circuit) to continuously flow.
This is why the shock received from static electricity is only a momentary jolt: the flow of
electrons is necessarily brief when static charges are equalized between two objects. Shocks of
self-limited duration like this are rarely hazardous.
Without two contact points on the body for current to enter and exit, respectively, there
is no hazard of shock. This is why birds can safely rest on high-voltage power lines without
getting shocked: they make contact with the circuit at only one point.
High voltage
across source
and load
bird (not shocked)
In order for electrons to flow through a conductor, there must be a voltage present to moti-
vate them. Voltage, as you should recall, is always relative between two points. There is no such
thing as voltage "on" or "at" a single point in the circuit, and so the bird contacting a single
point in the above circuit has no voltage applied across its body to establish a current through
it. Yes, even though they rest on two feet, both feet are touching the same wire, making them
electrically common. Electrically speaking, both of the bird's feet touch the same point, hence
there is no voltage between them to motivate current through the bird's body.
This might lend one to believe that its impossible to be shocked by electricity by only touch-
ing a single wire. Like the birds, if we're sure to touch only one wire at a time, we'll be safe,
right? Unfortunately, this is not correct. Unlike birds, people are usually standing on the
ground when they contact a "live" wire. Many times, one side of a power system will be inten-
tionally connected to earth ground, and so the person touching a single wire is actually making
contact between two points in the circuit (the wire and earth ground):
3.3.
SHOCK CURRENT PATH
81
High voltage
across source
and load
bird (not shocked)
path for current through the dirt
person (SHOCKED!)
The ground symbol is that set of three horizontal bars of decreasing width located at the
lower-left of the circuit shown, and also at the foot of the person being shocked. In real life the
power system ground consists of some kind of metallic conductor buried deep in the ground
for making maximum contact with the earth. That conductor is electrically connected to an
appropriate connection point on the circuit with thick wire. The victim's ground connection is
through their feet, which are touching the earth.
A few questions usually arise at this point in the mind of the student:
• If the presence of a ground point in the circuit provides an easy point of contact for some-
one to get shocked, why have it in the circuit at all? Wouldn't a ground-less circuit be
safer?
• The person getting shocked probably isn't bare-footed. If rubber and fabric are insulat-
ing materials, then why aren't their shoes protecting them by preventing a circuit from
forming?
• How good of a conductor can dirt be? If you can get shocked by current through the earth,
why not use the earth as a conductor in our power circuits?
In answer to the first question, the presence of an intentional "grounding" point in an elec-
tric circuit is intended to ensure that one side of it is safe to come in contact with. Note that
if our victim in the above diagram were to touch the bottom side of the resistor, nothing would
happen even though their feet would still be contacting ground:
82
CHAPTER 3. ELECTRICAL SAFETY
High voltage
across source
and load
bird (not shocked)
person (not shocked)
no current!
Because the bottom side of the circuit is firmly connected to ground through the grounding
point on the lower-left of the circuit, the lower conductor of the circuit is made electrically
common with earth ground. Since there can be no voltage between electrically common points,
there will be no voltage applied across the person contacting the lower wire, and they will
not receive a shock. For the same reason, the wire connecting the circuit to the grounding
rod/plates is usually left bare (no insulation), so that any metal object it brushes up against
will similarly be electrically common with the earth.
Circuit grounding ensures that at least one point in the circuit will be safe to touch. But
what about leaving a circuit completely ungrounded? Wouldn't that make any person touching
just a single wire as safe as the bird sitting on just one? Ideally, yes. Practically, no. Observe
what happens with no ground at all:
High voltage
across source
and load
bird (not shocked)
person (not shocked)
Despite the fact that the person's feet are still contacting ground, any single point in the
circuit should be safe to touch. Since there is no complete path (circuit) formed through the
person's body from the bottom side of the voltage source to the top, there is no way for a current
to be established through the person. However, this could all change with an accidental ground,
such as a tree branch touching a power line and providing connection to earth ground:
3.3.
SHOCK CURRENT PATH
83
High voltage
across source
and load
bird (not shocked)
person (SHOCKED!)
accidental ground path through tree
(touching wire) completes the circuit
for shock current through the victim.
Such an accidental connection between a power system conductor and the earth (ground) is
called a ground fault. Ground faults may be caused by many things, including dirt buildup on
power line insulators (creating a dirty-water path for current from the conductor to the pole,
and to the ground, when it rains), ground water infiltration in buried power line conductors,
and birds landing on power lines, bridging the line to the pole with their wings. Given the
many causes of ground faults, they tend to be unpredicatable. In the case of trees, no one can
guarantee which wire their branches might touch. If a tree were to brush up against the top
wire in the circuit, it would make the top wire safe to touch and the bottom one dangerous -
just the opposite of the previous scenario where the tree contacts the bottom wire:
84
CHAPTER 3. ELECTRICAL SAFETY
High voltage
across source
and load
bird (not shocked)
person (SHOCKED!)
accidental ground path through tree
(touching wire) completes the circuit
for shock current through the victim.
person (not shocked)
With a tree branch contacting the top wire, that wire becomes the grounded conductor in
the circuit, electrically common with earth ground. Therefore, there is no voltage between that
wire and ground, but full (high) voltage between the bottom wire and ground. As mentioned
previously, tree branches are only one potential source of ground faults in a power system.
Consider an ungrounded power system with no trees in contact, but this time with two people
touching single wires:
High voltage
across source
and load
bird (not shocked)
person (SHOCKED!)
person (SHOCKED!)
With each person standing on the ground, contacting different points in the circuit, a path
for shock current is made through one person, through the earth, and through the other person.
Even though each person thinks they're safe in only touching a single point in the circuit, their
combined actions create a deadly scenario. In effect, one person acts as the ground fault which
makes it unsafe for the other person. This is exactly why ungrounded power systems are
3.3.
SHOCK CURRENT PATH
85
dangerous: the voltage between any point in the circuit and ground (earth) is unpredictable,
because a ground fault could appear at any point in the circuit at any time. The only character
guaranteed to be safe in these scenarios is the bird, who has no connection to earth ground
at all! By firmly connecting a designated point in the circuit to earth ground ("grounding" the
circuit), at least safety can be assured at that one point. This is more assurance of safety than
having no ground connection at all.
In answer to the second question, rubber-soled shoes do indeed provide some electrical
insulation to help protect someone from conducting shock current through their feet. However,
most common shoe designs are not intended to be electrically "safe," their soles being too thin
and not of the right substance. Also, any moisture, dirt, or conductive salts from body sweat on
the surface of or permeated through the soles of shoes will compromise what little insulating
value the shoe had to begin with. There are shoes specifically made for dangerous electrical
work, as well as thick rubber mats made to stand on while working on live circuits, but these
special pieces of gear must be in absolutely clean, dry condition in order to be effective. Suffice
it to say, normal footwear is not enough to guarantee protection against electric shock from a
power system.
Research conducted on contact resistance between parts of the human body and points of
contact (such as the ground) shows a wide range of figures (see end of chapter for information
on the source of this data):
• Hand or foot contact, insulated with rubber: 20 MΩtypical.
• Foot contact through leather shoe sole (dry): 100 kΩto 500 kΩ
• Foot contact through leather shoe sole (wet): 5 kΩto 20 kΩ
As you can see, not only is rubber a far better insulating material than leather, but the
presence of water in a porous substance such as leather greatly reduces electrical resistance.
In answer to the third question, dirt is not a very good conductor (at least not when its
dry!). It is too poor of a conductor to support continuous current for powering a load. However,
as we will see in the next section, it takes very little current to injure or kill a human being, so
even the poor conductivity of dirt is enough to provide a path for deadly current when there is
sufficient voltage available, as there usually is in power systems.
Some ground surfaces are better insulators than others. Asphalt, for instance, being oil-
based, has a much greater resistance than most forms of dirt or rock. Concrete, on the other
hand, tends to have fairly low resistance due to its intrinsic water and electrolyte (conductive
chemical) content.
• REVIEW:
• Electric shock can only occur when contact is made between two points of a circuit; when
voltage is applied across a victim's body.
• Power circuits usually have a designated point that is "grounded:" firmly connected to
metal rods or plates buried in the dirt to ensure that one side of the circuit is always at
ground potential (zero voltage between that point and earth ground).
• A ground fault is an accidental connection between a circuit conductor and the earth
(ground).
86
CHAPTER 3. ELECTRICAL SAFETY
• Special, insulated shoes and mats are made to protect persons from shock via ground
conduction, but even these pieces of gear must be in clean, dry condition to be effective.
Normal footwear is not good enough to provide protection from shock by insulating its
wearer from the earth.
• Though dirt is a poor conductor, it can conduct enough current to injure or kill a human
being.
3.4
Ohm's Law (again!)
A common phrase heard in reference to electrical safety goes something like this: "It's not volt-
age that kills, its current!" While there is an element of truth to this, there's more to understand
about shock hazard than this simple adage. If voltage presented no danger, no one would ever
print and display signs saying: DANGER - HIGH VOLTAGE!
The principle that "current kills" is essentially correct. It is electric current that burns
tissue, freezes muscles, and fibrillates hearts. However, electric current doesn't just occur on
its own: there must be voltage available to motivate electrons to flow through a victim. A
person's body also presents resistance to current, which must be taken into account.
Taking Ohm's Law for voltage, current, and resistance, and expressing it in terms of current
for a given voltage and resistance, we have this equation:
Ohm's Law
E
R
Current =
Voltage
Resistance
The amount of current through a body is equal to the amount of voltage applied between
two points on that body, divided by the electrical resistance offered by the body between those
two points. Obviously, the more voltage available to cause electrons to flow, the easier they
will flow through any given amount of resistance. Hence, the danger of high voltage: high
voltage means potential for large amounts of current through your body, which will injure or
kill you. Conversely, the more resistance a body offers to current, the slower electrons will flow
for any given amount of voltage. Just how much voltage is dangerous depends on how much
total resistance is in the circuit to oppose the flow of electrons.
Body resistance is not a fixed quantity. It varies from person to person and from time to
time. There's even a body fat measurement technique based on a measurement of electrical
resistance between a person's toes and fingers. Differing percentages of body fat give different
resistances: just one variable affecting electrical resistance in the human body. In order for
the technique to work accurately, the person must regulate their fluid intake for several hours
prior to the test, indicating that body hydration is another factor impacting the body's electrical
resistance.
Body resistance also varies depending on how contact is made with the skin: is it from hand-
to-hand, hand-to-foot, foot-to-foot, hand-to-elbow, etc.? Sweat, being rich in salts and minerals,
is an excellent conductor of electricity for being a liquid. So is blood, with its similarly high
content of conductive chemicals. Thus, contact with a wire made by a sweaty hand or open
wound will offer much less resistance to current than contact made by clean, dry skin.
3.4.
OHM'S LAW (AGAIN!)
87
Measuring electrical resistance with a sensitive meter, I measure approximately 1 million
ohms of resistance (1 MΩ) between my two hands, holding on to the meter's metal probes
between my fingers. The meter indicates less resistance when I squeeze the probes tightly and
more resistance when I hold them loosely. Sitting here at my computer, typing these words,
my hands are clean and dry. If I were working in some hot, dirty, industrial environment, the
resistance between my hands would likely be much less, presenting less opposition to deadly
current, and a greater threat of electrical shock.
But how much current is harmful? The answer to that question also depends on several
factors. Individual body chemistry has a significant impact on how electric current affects an
individual. Some people are highly sensitive to current, experiencing involuntary muscle con-
traction with shocks from static electricity. Others can draw large sparks from discharging
static electricity and hardly feel it, much less experience a muscle spasm. Despite these dif-
ferences, approximate guidelines have been developed through tests which indicate very little
current being necessary to manifest harmful effects (again, see end of chapter for information
on the source of this data). All current figures given in milliamps (a milliamp is equal to 1/1000
of an amp):
BODILY EFFECT
DIRECT CURRENT (DC)
60 Hz AC
10 kHz AC
Slight sensation
Men = 1.0 mA
0.4 mA
7 mA
felt at hand(s)
Women = 0.6 mA
0.3 mA
5 mA
Threshold of
Men = 5.2 mA
1.1 mA
12 mA
perception
Women = 3.5 mA
0.7 mA
8 mA
Painful, but
Men = 62 mA
9 mA
55 mA
voluntary muscle
Women = 41 mA
6 mA
37 mA
control maintained
Painful, unable
Men = 76 mA
16 mA
75 mA
to let go of wires
Women = 51 mA
10.5 mA
50 mA
Severe pain,
Men = 90 mA
23 mA
94 mA
difficulty
Women = 60 mA
15 mA
63 mA
breathing
Possible heart
Men = 500 mA
100 mA
fibrillation
Women = 500 mA
100 mA
after 3 seconds
"Hz" stands for the unit of Hertz, the measure of how rapidly alternating current alternates,
a measure otherwise known as frequency. So, the column of figures labeled "60 Hz AC" refers
to current that alternates at a frequency of 60 cycles (1 cycle = period of time where electrons
flow one direction, then the other direction) per second. The last column, labeled "10 kHz AC,"
88
CHAPTER 3. ELECTRICAL SAFETY
refers to alternating current that completes ten thousand (10,000) back-and-forth cycles each
and every second.
Keep in mind that these figures are only approximate, as individuals with different body
chemistry may react differently. It has been suggested that an across-the-chest current of only
17 milliamps AC is enough to induce fibrillation in a human subject under certain conditions.
Most of our data regarding induced fibrillation comes from animal testing. Obviously, it is not
practical to perform tests of induced ventricular fibrillation on human subjects, so the available
data is sketchy. Oh, and in case you're wondering, I have no idea why women tend to be more
susceptible to electric currents than men!
Suppose I were to place my two hands across the terminals of an AC voltage source at
60 Hz (60 cycles, or alternations back-and-forth, per second). How much voltage would be
necessary in this clean, dry state of skin condition to produce a current of 20 milliamps (enough
to cause me to become unable to let go of the voltage source)? We can use Ohm's Law (E=IR) to
determine this:
E = (20 mA)(1 MΩ)
E = 20,000 volts, or 20 kV
Bear in mind that this is a "best case" scenario (clean, dry skin) from the standpoint of
electrical safety, and that this figure for voltage represents the amount necessary to induce
tetanus. Far less would be required to cause a painful shock! Also keep in mind that the
physiological effects of any particular amount of current can vary significantly from person to
person, and that these calculations are rough estimates only.
With water sprinkled on my fingers to simulate sweat, I was able to measure a hand-to-
hand resistance of only 17,000 ohms (17 kΩ). Bear in mind this is only with one finger of each
hand contacting a thin metal wire. Recalculating the voltage required to cause a current of 20
milliamps, we obtain this figure:
E = (20 mA)(17 kΩ)
E = 340 volts
In this realistic condition, it would only take 340 volts of potential from one of my hands
to the other to cause 20 milliamps of current. However, it is still possible to receive a deadly
shock from less voltage than this. Provided a much lower body resistance figure augmented
by contact with a ring (a band of gold wrapped around the circumference of one's finger makes
an excellent contact point for electrical shock) or full contact with a large metal object such as
a pipe or metal handle of a tool, the body resistance figure could drop as low as 1,000 ohms (1
kΩ), allowing an even lower voltage to present a potential hazard:
3.4.
OHM'S LAW (AGAIN!)
89
E = (20 mA)(1 kΩ)
E = 20 volts
Notice that in this condition, 20 volts is enough to produce a current of 20 milliamps through
a person: enough to induce tetanus. Remember, it has been suggested a current of only 17
milliamps may induce ventricular (heart) fibrillation. With a hand-to-hand resistance of 1000
Ω, it would only take 17 volts to create this dangerous condition:
E = (17 mA)(1 kΩ)
E = 17 volts
Seventeen volts is not very much as far as electrical systems are concerned. Granted, this
is a "worst-case" scenario with 60 Hz AC voltage and excellent bodily conductivity, but it does
stand to show how little voltage may present a serious threat under certain conditions.
The conditions necessary to produce 1,000 Ωof body resistance don't have to be as extreme
as what was presented, either (sweaty skin with contact made on a gold ring). Body resis-
tance may decrease with the application of voltage (especially if tetanus causes the victim to
maintain a tighter grip on a conductor) so that with constant voltage a shock may increase in
severity after initial contact. What begins as a mild shock - just enough to "freeze" a victim
so they can't let go - may escalate into something severe enough to kill them as their body
resistance decreases and current correspondingly increases.
Research has provided an approximate set of figures for electrical resistance of human
contact points under different conditions (see end of chapter for information on the source
of this data):
• Wire touched by finger: 40,000 Ωto 1,000,000 Ωdry, 4,000 Ωto 15,000 Ωwet.
• Wire held by hand: 15,000 Ωto 50,000 Ωdry, 3,000 Ωto 5,000 Ωwet.
• Metal pliers held by hand: 5,000 Ωto 10,000 Ωdry, 1,000 Ωto 3,000 Ωwet.
• Contact with palm of hand: 3,000 Ωto 8,000 Ωdry, 1,000 Ωto 2,000 Ωwet.
• 1.5 inch metal pipe grasped by one hand: 1,000 Ωto 3,000 Ωdry, 500 Ωto 1,500 Ωwet.
• 1.5 inch metal pipe grasped by two hands: 500 Ωto 1,500 kΩdry, 250 Ωto 750 Ωwet.
• Hand immersed in conductive liquid: 200 Ωto 500 Ω.
• Foot immersed in conductive liquid: 100 Ωto 300 Ω.
Note the resistance values of the two conditions involving a 1.5 inch metal pipe. The re-
sistance measured with two hands grasping the pipe is exactly one-half the resistance of one
hand grasping the pipe.
90
CHAPTER 3. ELECTRICAL SAFETY
1.5" metal pipe
2 kΩ
With two hands, the bodily contact area is twice as great as with one hand. This is an im-
portant lesson to learn: electrical resistance between any contacting objects diminishes with
increased contact area, all other factors being equal. With two hands holding the pipe, elec-
trons have two, parallel routes through which to flow from the pipe to the body (or vice-versa).
1.5" metal pipe
1 kΩ
Two 2 kΩ contact points in "parallel"
with each other gives 1 kΩ total
pipe-to-body resistance.
As we will see in a later chapter, parallel circuit pathways always result in less overall
resistance than any single pathway considered alone.
In industry, 30 volts is generally considered to be a conservative threshold value for dan-
gerous voltage. The cautious person should regard any voltage above 30 volts as threatening,
not relying on normal body resistance for protection against shock. That being said, it is still
an excellent idea to keep one's hands clean and dry, and remove all metal jewelry when work-
ing around electricity. Even around lower voltages, metal jewelry can present a hazard by
conducting enough current to burn the skin if brought into contact between two points in a cir-
cuit. Metal rings, especially, have been the cause of more than a few burnt fingers by bridging
between points in a low-voltage, high-current circuit.
Also, voltages lower than 30 can be dangerous if they are enough to induce an unpleasant
sensation, which may cause you to jerk and accidently come into contact across a higher voltage
or some other hazard. I recall once working on a automobile on a hot summer day. I was
wearing shorts, my bare leg contacting the chrome bumper of the vehicle as I tightened battery
connections. When I touched my metal wrench to the positive (ungrounded) side of the 12 volt
battery, I could feel a tingling sensation at the point where my leg was touching the bumper.
The combination of firm contact with metal and my sweaty skin made it possible to feel a shock
with only 12 volts of electrical potential.
3.4.
OHM'S LAW (AGAIN!)
91
Thankfully, nothing bad happened, but had the engine been running and the shock felt at
my hand instead of my leg, I might have reflexively jerked my arm into the path of the rotating
fan, or dropped the metal wrench across the battery terminals (producing large amounts of
current through the wrench with lots of accompanying sparks). This illustrates another im-
portant lesson regarding electrical safety; that electric current itself may be an indirect cause
of injury by causing you to jump or spasm parts of your body into harm's way.
The path current takes through the human body makes a difference as to how harmful it is.
Current will affect whatever muscles are in its path, and since the heart and lung (diaphragm)
muscles are probably the most critical to one's survival, shock paths traversing the chest are
the most dangerous. This makes the hand-to-hand shock current path a very likely mode of
injury and fatality.
To guard against such an occurrence, it is advisable to only use one hand to work on live
circuits of hazardous voltage, keeping the other hand tucked into a pocket so as to not acci-
dently touch anything. Of course, it is always safer to work on a circuit when it is unpowered,
but this is not always practical or possible. For one-handed work, the right hand is generally
preferred over the left for two reasons: most people are right-handed (thus granting additional
coordination when working), and the heart is usually situated to the left of center in the chest
cavity.
For those who are left-handed, this advice may not be the best. If such a person is suffi-
ciently uncoordinated with their right hand, they may be placing themselves in greater danger
by using the hand they're least comfortable with, even if shock current through that hand
might present more of a hazard to their heart. The relative hazard between shock through one
hand or the other is probably less than the hazard of working with less than optimal coordina-
tion, so the choice of which hand to work with is best left to the individual.
The best protection against shock from a live circuit is resistance, and resistance can be
added to the body through the use of insulated tools, gloves, boots, and other gear. Current in
a circuit is a function of available voltage divided by the total resistance in the path of the flow.
As we will investigate in greater detail later in this book, resistances have an additive effect
when they're stacked up so that there's only one path for electrons to flow:
Body resistance
I
I
E
Rbody
Person in direct contact with voltage source:
current limited only by body resistance.
Now we'll see an equivalent circuit for a person wearing insulated gloves and boots:
92
CHAPTER 3. ELECTRICAL SAFETY
Body resistance
I
I
Glove resistance
Boot resistance
E
Rbody
Person wearing insulating gloves and boots:
current now limited by total circuit resistance.
Rglove
Rboot
Because electric current must pass through the boot and the body and the glove to complete
its circuit back to the battery, the combined total (sum) of these resistances opposes the flow of
electrons to a greater degree than any of the resistances considered individually.
Safety is one of the reasons electrical wires are usually covered with plastic or rubber in-
sulation: to vastly increase the amount of resistance between the conductor and whoever or
whatever might contact it. Unfortunately, it would be prohibitively expensive to enclose power
line conductors in sufficient insulation to provide safety in case of accidental contact, so safety
is maintained by keeping those lines far enough out of reach so that no one can accidently
touch them.
• REVIEW:
• Harm to the body is a function of the amount of shock current. Higher voltage allows for
the production of higher, more dangerous currents. Resistance opposes current, making
high resistance a good protective measure against shock.
• Any voltage above 30 is generally considered to be capable of delivering dangerous shock
currents.
• Metal jewelry is definitely bad to wear when working around electric circuits. Rings,
watchbands, necklaces, bracelets, and other such adornments provide excellent electri-
cal contact with your body, and can conduct current themselves enough to produce skin
burns, even with low voltages.
• Low voltages can still be dangerous even if they're too low to directly cause shock in-
jury. They may be enough to startle the victim, causing them to jerk back and contact
something more dangerous in the near vicinity.
• When necessary to work on a "live" circuit, it is best to perform the work with one hand
so as to prevent a deadly hand-to-hand (through the chest) shock current path.
3.5.
SAFE PRACTICES
93
3.5
Safe practices
If at all possible, shut off the power to a circuit before performing any work on it. You must
secure all sources of harmful energy before a system may be considered safe to work on. In
industry, securing a circuit, device, or system in this condition is commonly known as placing
it in a Zero Energy State. The focus of this lesson is, of course, electrical safety. However, many
of these principles apply to non-electrical systems as well.
Securing something in a Zero Energy State means ridding it of any sort of potential or
stored energy, including but not limited to:
• Dangerous voltage
• Spring pressure
• Hydraulic (liquid) pressure
• Pneumatic (air) pressure
• Suspended weight
• Chemical energy (flammable or otherwise reactive substances)
• Nuclear energy (radioactive or fissile substances)
Voltage by its very nature is a manifestation of potential energy. In the first chapter I
even used elevated liquid as an analogy for the potential energy of voltage, having the capacity
(potential) to produce current (flow), but not necessarily realizing that potential until a suitable
path for flow has been established, and resistance to flow is overcome. A pair of wires with
high voltage between them do not look or sound dangerous even though they harbor enough
potential energy between them to push deadly amounts of current through your body. Even
though that voltage isn't presently doing anything, it has the potential to, and that potential
must be neutralized before it is safe to physically contact those wires.
All properly designed circuits have "disconnect" switch mechanisms for securing voltage
from a circuit. Sometimes these "disconnects" serve a dual purpose of automatically opening
under excessive current conditions, in which case we call them "circuit breakers." Other times,
the disconnecting switches are strictly manually-operated devices with no automatic function.
In either case, they are there for your protection and must be used properly. Please note that
the disconnect device should be separate from the regular switch used to turn the device on
and off. It is a safety switch, to be used only for securing the system in a Zero Energy State:
Power
source
Load
Disconnect
switch
switch
94
CHAPTER 3. ELECTRICAL SAFETY
With the disconnect switch in the "open" position as shown (no continuity), the circuit is
broken and no current will exist. There will be zero voltage across the load, and the full voltage
of the source will be dropped across the open contacts of the disconnect switch. Note how there
is no need for a disconnect switch in the lower conductor of the circuit. Because that side of
the circuit is firmly connected to the earth (ground), it is electrically common with the earth
and is best left that way. For maximum safety of personnel working on the load of this circuit,
a temporary ground connection could be established on the top side of the load, to ensure that
no voltage could ever be dropped across the load:
Power
source
Load
Disconnect
switch
switch
temporary
ground
With the temporary ground connection in place, both sides of the load wiring are connected
to ground, securing a Zero Energy State at the load.
Since a ground connection made on both sides of the load is electrically equivalent to short-
circuiting across the load with a wire, that is another way of accomplishing the same goal of
maximum safety:
Power
source
Load
Disconnect
switch
switch
temporary
shorting wire
zero voltage
ensured here
Either way, both sides of the load will be electrically common to the earth, allowing for no
voltage (potential energy) between either side of the load and the ground people stand on. This
technique of temporarily grounding conductors in a de-energized power system is very common
in maintenance work performed on high voltage power distribution systems.
A further benefit of this precaution is protection against the possibility of the disconnect
switch being closed (turned "on" so that circuit continuity is established) while people are still
contacting the load. The temporary wire connected across the load would create a short-circuit
when the disconnect switch was closed, immediately tripping any overcurrent protection de-
vices (circuit breakers or fuses) in the circuit, which would shut the power off again. Damage
may very well be sustained by the disconnect switch if this were to happen, but the workers at
3.5.
SAFE PRACTICES
95
the load are kept safe.
It would be good to mention at this point that overcurrent devices are not intended to
provide protection against electric shock. Rather, they exist solely to protect conductors from
overheating due to excessive currents. The temporary shorting wires just described would
indeed cause any overcurrent devices in the circuit to "trip" if the disconnect switch were to be
closed, but realize that electric shock protection is not the intended function of those devices.
Their primary function would merely be leveraged for the purpose of worker protection with
the shorting wire in place.
Since it is obviously important to be able to secure any disconnecting devices in the open
(off) position and make sure they stay that way while work is being done on the circuit, there
is need for a structured safety system to be put into place. Such a system is commonly used in
industry and it is called Lock-out/Tag-out.
A lock-out/tag-out procedure works like this: all individuals working on a secured circuit
have their own personal padlock or combination lock which they set on the control lever of
a disconnect device prior to working on the system. Additionally, they must fill out and sign
a tag which they hang from their lock describing the nature and duration of the work they
intend to perform on the system. If there are multiple sources of energy to be "locked out"
(multiple disconnects, both electrical and mechanical energy sources to be secured, etc.), the
worker must use as many of his or her locks as necessary to secure power from the system
before work begins. This way, the system is maintained in a Zero Energy State until every last
lock is removed from all the disconnect and shutoff devices, and that means every last worker
gives consent by removing their own personal locks. If the decision is made to re-energize the
system and one person's lock(s) still remain in place after everyone present removes theirs, the
tag(s) will show who that person is and what it is they're doing.
Even with a good lock-out/tag-out safety program in place, there is still need for diligence
and common-sense precaution. This is especially true in industrial settings where a multitude
of people may be working on a device or system at once. Some of those people might not know
about proper lock-out/tag-out procedure, or might know about it but are too complacent to
follow it. Don't assume that everyone has followed the safety rules!
After an electrical system has been locked out and tagged with your own personal lock, you
must then double-check to see if the voltage really has been secured in a zero state. One way
to check is to see if the machine (or whatever it is that's being worked on) will start up if the
Start switch or button is actuated. If it starts, then you know you haven't successfully secured
the electrical power from it.
Additionally, you should always check for the presence of dangerous voltage with a mea-
suring device before actually touching any conductors in the circuit. To be safest, you should
follow this procedure of checking, using, and then checking your meter:
• Check to see that your meter indicates properly on a known source of voltage.
• Use your meter to test the locked-out circuit for any dangerous voltage.
• Check your meter once more on a known source of voltage to see that it still indicates as
it should.
While this may seem excessive or even paranoid, it is a proven technique for preventing
electrical shock. I once had a meter fail to indicate voltage when it should have while checking
96
CHAPTER 3. ELECTRICAL SAFETY
a circuit to see if it was "dead." Had I not used other means to check for the presence of voltage,
I might not be alive today to write this. There's always the chance that your voltage meter will
be defective just when you need it to check for a dangerous condition. Following these steps
will help ensure that you're never misled into a deadly situation by a broken meter.
Finally, the electrical worker will arrive at a point in the safety check procedure where it is
deemed safe to actually touch the conductor(s). Bear in mind that after all of the precautionary
steps have taken, it is still possible (although very unlikely) that a dangerous voltage may be
present. One final precautionary measure to take at this point is to make momentary contact
with the conductor(s) with the back of the hand before grasping it or a metal tool in contact
with it. Why? If, for some reason there is still voltage present between that conductor and
earth ground, finger motion from the shock reaction (clenching into a fist) will break contact
with the conductor. Please note that this is absolutely the last step that any electrical worker
should ever take before beginning work on a power system, and should never be used as an
alternative method of checking for dangerous voltage. If you ever have reason to doubt the
trustworthiness of your meter, use another meter to obtain a "second opinion."
• REVIEW:
• Zero Energy State: When a circuit, device, or system has been secured so that no potential
energy exists to harm someone working on it.
• Disconnect switch devices must be present in a properly designed electrical system to
allow for convenient readiness of a Zero Energy State.
• Temporary grounding or shorting wires may be connected to a load being serviced for
extra protection to personnel working on that load.
• Lock-out/Tag-out works like this: when working on a system in a Zero Energy State, the
worker places a personal padlock or combination lock on every energy disconnect device
relevant to his or her task on that system. Also, a tag is hung on every one of those locks
describing the nature and duration of the work to be done, and who is doing it.
• Always verify that a circuit has been secured in a Zero Energy State with test equipment
after "locking it out." Be sure to test your meter before and after checking the circuit to
verify that it is working properly.
• When the time comes to actually make contact with the conductor(s) of a supposedly dead
power system, do so first with the back of one hand, so that if a shock should occur, the
muscle reaction will pull the fingers away from the conductor.
3.6
Emergency response
Despite lock-out/tag-out procedures and multiple repetitions of electrical safety rules in indus-
try, accidents still do occur. The vast majority of the time, these accidents are the result of not
following proper safety procedures. But however they may occur, they still do happen, and any-
one working around electrical systems should be aware of what needs to be done for a victim
of electrical shock.
3.6.
EMERGENCY RESPONSE
97
If you see someone lying unconscious or "froze on the circuit," the very first thing to do is
shut off the power by opening the appropriate disconnect switch or circuit breaker. If someone
touches another person being shocked, there may be enough voltage dropped across the body
of the victim to shock the would-be rescuer, thereby "freezing" two people instead of one. Don't
be a hero. Electrons don't respect heroism. Make sure the situation is safe for you to step into,
or else you will be the next victim, and nobody will benefit from your efforts.
One problem with this rule is that the source of power may not be known, or easily found
in time to save the victim of shock. If a shock victim's breathing and heartbeat are paralyzed
by electric current, their survival time is very limited. If the shock current is of sufficient
magnitude, their flesh and internal organs may be quickly roasted by the power the current
dissipates as it runs through their body.
If the power disconnect switch cannot be located quickly enough, it may be possible to dis-
lodge the victim from the circuit they're frozen on to by prying them or hitting them away with
a dry wooden board or piece of nonmetallic conduit, common items to be found in industrial
construction scenes. Another item that could be used to safely drag a "frozen" victim away from
contact with power is an extension cord. By looping a cord around their torso and using it as
a rope to pull them away from the circuit, their grip on the conductor(s) may be broken. Bear
in mind that the victim will be holding on to the conductor with all their strength, so pulling
them away probably won't be easy!
Once the victim has been safely disconnected from the source of electric power, the im-
mediate medical concerns for the victim should be respiration and circulation (breathing and
pulse). If the rescuer is trained in CPR, they should follow the appropriate steps of checking
for breathing and pulse, then applying CPR as necessary to keep the victim's body from deoxy-
genating. The cardinal rule of CPR is to keep going until you have been relieved by qualified
personnel.
If the victim is conscious, it is best to have them lie still until qualified emergency response
personnel arrive on the scene. There is the possibility of the victim going into a state of physio-
logical shock - a condition of insufficient blood circulation different from electrical shock - and
so they should be kept as warm and comfortable as possible. An electrical shock insufficient
to cause immediate interruption of the heartbeat may be strong enough to cause heart irregu-
larities or a heart attack up to several hours later, so the victim should pay close attention to
their own condition after the incident, ideally under supervision.
• REVIEW:
• A person being shocked needs to be disconnected from the source of electrical power.
Locate the disconnecting switch/breaker and turn it off. Alternatively, if the disconnecting
device cannot be located, the victim can be pried or pulled from the circuit by an insulated
object such as a dry wood board, piece of nonmetallic conduit, or rubber electrical cord.
• Victims need immediate medical response: check for breathing and pulse, then apply
CPR as necessary to maintain oxygenation.
• If a victim is still conscious after having been shocked, they need to be closely monitored
and cared for until trained emergency response personnel arrive.
There is danger of
physiological shock, so keep the victim warm and comfortable.
98
CHAPTER 3. ELECTRICAL SAFETY
• Shock victims may suffer heart trouble up to several hours after being shocked. The
danger of electric shock does not end after the immediate medical attention.
3.7
Common sources of hazard
Of course there is danger of electrical shock when directly performing manual work on an
electrical power system. However, electric shock hazards exist in many other places, thanks to
the widespread use of electric power in our lives.
As we saw earlier, skin and body resistance has a lot to do with the relative hazard of
electric circuits. The higher the body's resistance, the less likely harmful current will result
from any given amount of voltage. Conversely, the lower the body's resistance, the more likely
for injury to occur from the application of a voltage.
The easiest way to decrease skin resistance is to get it wet. Therefore, touching electrical
devices with wet hands, wet feet, or especially in a sweaty condition (salt water is a much better
conductor of electricity than fresh water) is dangerous. In the household, the bathroom is one
of the more likely places where wet people may contact electrical appliances, and so shock
hazard is a definite threat there. Good bathroom design will locate power receptacles away
from bathtubs, showers, and sinks to discourage the use of appliances nearby. Telephones that
plug into a wall socket are also sources of hazardous voltage (the open circuit voltage is 48 volts
DC, and the ringing signal is 150 volts AC - remember that any voltage over 30 is considered
potentially dangerous!). Appliances such as telephones and radios should never, ever be used
while sitting in a bathtub. Even battery-powered devices should be avoided. Some battery-
operated devices employ voltage-increasing circuitry capable of generating lethal potentials.
Swimming pools are another source of trouble, since people often operate radios and other
powered appliances nearby. The National Electrical Code requires that special shock-detecting
receptacles called Ground-Fault Current Interrupting (GFI or GFCI) be installed in wet and
outdoor areas to help prevent shock incidents. More on these devices in a later section of this
chapter. These special devices have no doubt saved many lives, but they can be no substitute
for common sense and diligent precaution. As with firearms, the best "safety" is an informed
and conscientious operator.
Extension cords, so commonly used at home and in industry, are also sources of potential
hazard. All cords should be regularly inspected for abrasion or cracking of insulation, and
repaired immediately. One sure method of removing a damaged cord from service is to unplug
it from the receptacle, then cut off that plug (the "male" plug) with a pair of side-cutting pliers
to ensure that no one can use it until it is fixed. This is important on jobsites, where many
people share the same equipment, and not all people there may be aware of the hazards.
Any power tool showing evidence of electrical problems should be immediately serviced
as well.
I've heard several horror stories of people who continue to work with hand tools
that periodically shock them. Remember, electricity can kill, and the death it brings can be
gruesome. Like extension cords, a bad power tool can be removed from service by unplugging
it and cutting off the plug at the end of the cord.
Downed power lines are an obvious source of electric shock hazard and should be avoided
at all costs. The voltages present between power lines or between a power line and earth
ground are typically very high (2400 volts being one of the lowest voltages used in residential
distribution systems). If a power line is broken and the metal conductor falls to the ground,
3.7.
COMMON SOURCES OF HAZARD
99
the immediate result will usually be a tremendous amount of arcing (sparks produced), often
enough to dislodge chunks of concrete or asphalt from the road surface, and reports rivaling
that of a rifle or shotgun. To come into direct contact with a downed power line is almost sure
to cause death, but other hazards exist which are not so obvious.
When a line touches the ground, current travels between that downed conductor and the
nearest grounding point in the system, thus establishing a circuit:
downed power line
current through the earth
The earth, being a conductor (if only a poor one), will conduct current between the downed
line and the nearest system ground point, which will be some kind of conductor buried in the
ground for good contact. Being that the earth is a much poorer conductor of electricity than the
metal cables strung along the power poles, there will be substantial voltage dropped between
the point of cable contact with the ground and the grounding conductor, and little voltage
dropped along the length of the cabling (the following figures are very approximate):
downed power line
current through the earth
2400
volts
2390
volts
10
volts
If the distance between the two ground contact points (the downed cable and the system
ground) is small, there will be substantial voltage dropped along short distances between the
two points. Therefore, a person standing on the ground between those two points will be in
danger of receiving an electric shock by intercepting a voltage between their two feet!
100
CHAPTER 3. ELECTRICAL SAFETY
downed power line
current through the earth
2400
volts
2390
volts
10
volts
250 volts
person
(SHOCKED!)
Again, these voltage figures are very approximate, but they serve to illustrate a potential
hazard: that a person can become a victim of electric shock from a downed power line without
even coming into contact with that line!
One practical precaution a person could take if they see a power line falling towards the
ground is to only contact the ground at one point, either by running away (when you run, only
one foot contacts the ground at any given time), or if there's nowhere to run, by standing on one
foot. Obviously, if there's somewhere safer to run, running is the best option. By eliminating
two points of contact with the ground, there will be no chance of applying deadly voltage across
the body through both legs.
• REVIEW:
• Wet conditions increase risk of electric shock by lowering skin resistance.
• Immediately replace worn or damaged extension cords and power tools. You can prevent
innocent use of a bad cord or tool by cutting the male plug off the cord (while its unplugged
from the receptacle, of course).
• Power lines are very dangerous and should be avoided at all costs. If you see a line about
to hit the ground, stand on one foot or run (only one foot contacting the ground) to prevent
shock from voltage dropped across the ground between the line and the system ground
point.
3.8
Safe circuit design
As we saw earlier, a power system with no secure connection to earth ground is unpredictable
from a safety perspective: there's no way to guarantee how much or how little voltage will exist
between any point in the circuit and earth ground. By grounding one side of the power system's
voltage source, at least one point in the circuit can be assured to be electrically common with
the earth and therefore present no shock hazard. In a simple two-wire electrical power system,
the conductor connected to ground is called the neutral, and the other conductor is called the
hot, also known as the live or the active:
3.8.
SAFE CIRCUIT DESIGN
101
Source
Load
"Hot" conductor
"Neutral" conductor
Ground point
As far as the voltage source and load are concerned, grounding makes no difference at all.
It exists purely for the sake of personnel safety, by guaranteeing that at least one point in the
circuit will be safe to touch (zero voltage to ground). The "Hot" side of the circuit, named for
its potential for shock hazard, will be dangerous to touch unless voltage is secured by proper
disconnection from the source (ideally, using a systematic lock-out/tag-out procedure).
This imbalance of hazard between the two conductors in a simple power circuit is important
to understand. The following series of illustrations are based on common household wiring
systems (using DC voltage sources rather than AC for simplicity).
If we take a look at a simple, household electrical appliance such as a toaster with a conduc-
tive metal case, we can see that there should be no shock hazard when it is operating properly.
The wires conducting power to the toaster's heating element are insulated from touching the
metal case (and each other) by rubber or plastic.
Source
Ground point
"Hot"
"Neutral"
120 V
plug
no voltage
between case
and ground
metal case
Electrical
appliance
However, if one of the wires inside the toaster were to accidently come in contact with the
metal case, the case will be made electrically common to the wire, and touching the case will
be just as hazardous as touching the wire bare. Whether or not this presents a shock hazard
depends on which wire accidentally touches:
102
CHAPTER 3. ELECTRICAL SAFETY
Source
Ground point
"Hot"
"Neutral"
120 V
plug
accidental
contact
voltage between
case and ground!
If the "hot" wire contacts the case, it places the user of the toaster in danger. On the other
hand, if the neutral wire contacts the case, there is no danger of shock:
Source
Ground point
"Hot"
"Neutral"
120 V
plug
accidental
contact
case and ground!
no voltage between
To help ensure that the former failure is less likely than the latter, engineers try to design
appliances in such a way as to minimize hot conductor contact with the case. Ideally, of course,
you don't want either wire accidently coming in contact with the conductive case of the appli-
ance, but there are usually ways to design the layout of the parts to make accidental contact
less likely for one wire than for the other. However, this preventative measure is effective only
if power plug polarity can be guaranteed. If the plug can be reversed, then the conductor more
likely to contact the case might very well be the "hot" one:
Source
Ground point
"Hot"
"Neutral"
120 V
plug
accidental
contact
case and ground!
voltage between
Appliances designed this way usually come with "polarized" plugs, one prong of the plug
being slightly narrower than the other. Power receptacles are also designed like this, one slot
being narrower than the other. Consequently, the plug cannot be inserted "backwards," and
3.8.
SAFE CIRCUIT DESIGN
103
conductor identity inside the appliance can be guaranteed. Remember that this has no effect
whatsoever on the basic function of the appliance: its strictly for the sake of user safety.
Some engineers address the safety issue simply by making the outside case of the appliance
nonconductive. Such appliances are called double-insulated, since the insulating case serves
as a second layer of insulation above and beyond that of the conductors themselves. If a wire
inside the appliance accidently comes in contact with the case, there is no danger presented to
the user of the appliance.
Other engineers tackle the problem of safety by maintaining a conductive case, but using a
third conductor to firmly connect that case to ground:
Source
Ground point
"Hot"
"Neutral"
120 V
plug
"Ground"
Grounded case
voltage between
case and ground
ensures zero
The third prong on the power cord provides a direct electrical connection from the appliance
case to earth ground, making the two points electrically common with each other. If they're
electrically common, then there cannot be any voltage dropped between them. At least, that's
how it is supposed to work. If the hot conductor accidently touches the metal appliance case, it
will create a direct short-circuit back to the voltage source through the ground wire, tripping
any overcurrent protection devices. The user of the appliance will remain safe.
This is why its so important never to cut the third prong off a power plug when trying to
fit it into a two-prong receptacle. If this is done, there will be no grounding of the appliance
case to keep the user(s) safe.
The appliance will still function properly, but if there is an
internal fault bringing the hot wire in contact with the case, the results can be deadly. If a
two-prong receptacle must be used, a two- to three-prong receptacle adapter can be installed
with a grounding wire attached to the receptacle's grounded cover screw. This will maintain
the safety of the grounded appliance while plugged in to this type of receptacle.
Electrically safe engineering doesn't necessarily end at the load, however. A final safeguard
against electrical shock can be arranged on the power supply side of the circuit rather than the
appliance itself. This safeguard is called ground-fault detection, and it works like this:
104
CHAPTER 3. ELECTRICAL SAFETY
Source
Ground point
"Hot"
"Neutral"
120 V
no voltage
between case
and ground
I
I
In a properly functioning appliance (shown above), the current measured through the hot
conductor should be exactly equal to the current through the neutral conductor, because there's
only one path for electrons to flow in the circuit. With no fault inside the appliance, there is
no connection between circuit conductors and the person touching the case, and therefore no
shock.
If, however, the hot wire accidently contacts the metal case, there will be current through
the person touching the case. The presence of a shock current will be manifested as a difference
of current between the two power conductors at the receptacle:
Source
"Hot"
"Neutral"
120 V
I
I
Shock current
(more)
(less)
accidental
contact
Shock current
Shock current
This difference in current between the "hot" and "neutral" conductors will only exist if there
is current through the ground connection, meaning that there is a fault in the system. There-
fore, such a current difference can be used as a way to detect a fault condition. If a device is
set up to measure this difference of current between the two power conductors, a detection of
current imbalance can be used to trigger the opening of a disconnect switch, thus cutting power
off and preventing serious shock:
3.8.
SAFE CIRCUIT DESIGN
105
Source
"Hot"
"Neutral"
120 V
I
I
switches open automatically
if the difference between the
two currents becomes too
great.
Such devices are called Ground Fault Current Interruptors, or GFCIs for short. Outside
North America, the GFCI is variously known as a safety switch, a residual current device
(RCD), an RCBO or RCD/MCB if combined with a miniature circuit breaker, or earth leakage
circuit breaker (ELCB). They are compact enough to be built into a power receptacle. These
receptacles are easily identified by their distinctive "Test" and "Reset" buttons. The big advan-
tage with using this approach to ensure safety is that it works regardless of the appliance's
design. Of course, using a double-insulated or grounded appliance in addition to a GFCI recep-
tacle would be better yet, but its comforting to know that something can be done to improve
safety above and beyond the design and condition of the appliance.
The arc fault circuit interrupter (AFCI), a circuit breaker designed to prevent fires, is de-
signed to open on intermittent resistive short circuits. For example, a normal 15 A breaker
is designed to open circuit quickly if loaded well beyond the 15 A rating, more slowly a little
beyond the rating. While this protects against direct shorts and several seconds of overload,
respectively, it does not protect against arcs- similar to arc-welding. An arc is a highly variable
load, repetitively peaking at over 70 A, open circuiting with alternating current zero-crossings.
Though, the average current is not enough to trip a standard breaker, it is enough to start a
fire. This arc could be created by a metalic short circuit which burns the metal open, leaving a
resistive sputtering plasma of ionized gases.
The AFCI contains electronic circuitry to sense this intermittent resistive short circuit.
It protects against both hot to neutral and hot to ground arcs. The AFCI does not protect
against personal shock hazards like a GFCI does. Thus, GFCIs still need to be installed in
kitchen, bath, and outdoors circuits. Since the AFCI often trips upon starting large motors,
and more generally on brushed motors, its installation is limited to bedroom circuits by the
U.S. National Electrical code. Use of the AFCI should reduce the number of electrical fires.
However, nuisance-trips when running appliances with motors on AFCI circuits is a problem.
• REVIEW:
• Power systems often have one side of the voltage supply connected to earth ground to
ensure safety at that point.
• The "grounded" conductor in a power system is called the neutral conductor, while the
ungrounded conductor is called the hot.
• Grounding in power systems exists for the sake of personnel safety, not the operation of
the load(s).
106
CHAPTER 3. ELECTRICAL SAFETY
• Electrical safety of an appliance or other load can be improved by good engineering: polar-
ized plugs, double insulation, and three-prong "grounding" plugs are all ways that safety
can be maximized on the load side.
• Ground Fault Current Interruptors (GFCIs) work by sensing a difference in current be-
tween the two conductors supplying power to the load. There should be no difference in
current at all. Any difference means that current must be entering or exiting the load by
some means other than the two main conductors, which is not good. A significant current
difference will automatically open a disconnecting switch mechanism, cutting power off
completely.
3.9
Safe meter usage
Using an electrical meter safely and efficiently is perhaps the most valuable skill an electronics
technician can master, both for the sake of their own personal safety and for proficiency at
their trade. It can be daunting at first to use a meter, knowing that you are connecting it to
live circuits which may harbor life-threatening levels of voltage and current. This concern is
not unfounded, and it is always best to proceed cautiously when using meters. Carelessness
more than any other factor is what causes experienced technicians to have electrical accidents.
The most common piece of electrical test equipment is a meter called the multimeter. Multi-
meters are so named because they have the ability to measure a multiple of variables: voltage,
current, resistance, and often many others, some of which cannot be explained here due to their
complexity. In the hands of a trained technician, the multimeter is both an efficient work tool
and a safety device. In the hands of someone ignorant and/or careless, however, the multimeter
may become a source of danger when connected to a "live" circuit.
There are many different brands of multimeters, with multiple models made by each man-
ufacturer sporting different sets of features. The multimeter shown here in the following il-
lustrations is a "generic" design, not specific to any manufacturer, but general enough to teach
the basic principles of use:
COM
A
V
V
A
A
OFF
Multimeter
You will notice that the display of this meter is of the "digital" type: showing numerical
values using four digits in a manner similar to a digital clock. The rotary selector switch
3.9.
SAFE METER USAGE
107
(now set in the Off position) has five different measurement positions it can be set in: two
"V" settings, two "A" settings, and one setting in the middle with a funny-looking "horseshoe"
symbol on it representing "resistance." The "horseshoe" symbol is the Greek letter "Omega"
(Ω), which is the common symbol for the electrical unit of ohms.
Of the two "V" settings and two "A" settings, you will notice that each pair is divided into
unique markers with either a pair of horizontal lines (one solid, one dashed), or a dashed line
with a squiggly curve over it. The parallel lines represent "DC" while the squiggly curve repre-
sents "AC." The "V" of course stands for "voltage" while the "A" stands for "amperage" (current).
The meter uses different techniques, internally, to measure DC than it uses to measure AC,
and so it requires the user to select which type of voltage (V) or current (A) is to be measured.
Although we haven't discussed alternating current (AC) in any technical detail, this distinction
in meter settings is an important one to bear in mind.
There are three different sockets on the multimeter face into which we can plug our test
leads. Test leads are nothing more than specially-prepared wires used to connect the meter
to the circuit under test. The wires are coated in a color-coded (either black or red) flexible
insulation to prevent the user's hands from contacting the bare conductors, and the tips of the
probes are sharp, stiff pieces of wire:
COM
A
V
V
A
A
OFF
tip
tip
probe
probe
lead
lead
plug
plug
The black test lead always plugs into the black socket on the multimeter: the one marked
"COM" for "common." The red test lead plugs into either the red socket marked for voltage and
resistance, or the red socket marked for current, depending on which quantity you intend to
108
CHAPTER 3. ELECTRICAL SAFETY
measure with the multimeter.
To see how this works, let's look at a couple of examples showing the meter in use. First,
we'll set up the meter to measure DC voltage from a battery:
COM
A
V
V
A
A
OFF
9
volts
Note that the two test leads are plugged into the appropriate sockets on the meter for
voltage, and the selector switch has been set for DC "V". Now, we'll take a look at an example
of using the multimeter to measure AC voltage from a household electrical power receptacle
(wall socket):
COM
A
V
V
A
A
OFF
The only difference in the setup of the meter is the placement of the selector switch: it is
now turned to AC "V". Since we're still measuring voltage, the test leads will remain plugged
in the same sockets. In both of these examples, it is imperative that you not let the probe tips
come in contact with one another while they are both in contact with their respective points on
the circuit. If this happens, a short-circuit will be formed, creating a spark and perhaps even a
ball of flame if the voltage source is capable of supplying enough current! The following image
illustrates the potential for hazard:
3.9.
SAFE METER USAGE
109
COM
A
V
V
A
A
OFF
large spark
from short-
circuit!
This is just one of the ways that a meter can become a source of hazard if used improperly.
Voltage measurement is perhaps the most common function a multimeter is used for. It
is certainly the primary measurement taken for safety purposes (part of the lock-out/tag-out
procedure), and it should be well understood by the operator of the meter. Being that voltage
is always relative between two points, the meter must be firmly connected to two points in a
circuit before it will provide a reliable measurement. That usually means both probes must be
grasped by the user's hands and held against the proper contact points of a voltage source or
circuit while measuring.
Because a hand-to-hand shock current path is the most dangerous, holding the meter probes
on two points in a high-voltage circuit in this manner is always a potential hazard. If the
protective insulation on the probes is worn or cracked, it is possible for the user's fingers to
come into contact with the probe conductors during the time of test, causing a bad shock to
occur. If it is possible to use only one hand to grasp the probes, that is a safer option. Sometimes
it is possible to "latch" one probe tip onto the circuit test point so that it can be let go of and
the other probe set in place, using only one hand. Special probe tip accessories such as spring
clips can be attached to help facilitate this.
Remember that meter test leads are part of the whole equipment package, and that they
should be treated with the same care and respect that the meter itself is. If you need a special
accessory for your test leads, such as a spring clip or other special probe tip, consult the product
catalog of the meter manufacturer or other test equipment manufacturer. Do not try to be
creative and make your own test probes, as you may end up placing yourself in danger the
next time you use them on a live circuit.
Also, it must be remembered that digital multimeters usually do a good job of discriminat-
ing between AC and DC measurements, as they are set for one or the other when checking
for voltage or current. As we have seen earlier, both AC and DC voltages and currents can be
deadly, so when using a multimeter as a safety check device you should always check for the
presence of both AC and DC, even if you're not expecting to find both! Also, when checking for
the presence of hazardous voltage, you should be sure to check all pairs of points in question.
For example, suppose that you opened up an electrical wiring cabinet to find three large
conductors supplying AC power to a load. The circuit breaker feeding these wires (supposedly)
has been shut off, locked, and tagged. You double-checked the absence of power by pressing the
110
CHAPTER 3. ELECTRICAL SAFETY
Start button for the load. Nothing happened, so now you move on to the third phase of your
safety check: the meter test for voltage.
First, you check your meter on a known source of voltage to see that its working properly.
Any nearby power receptacle should provide a convenient source of AC voltage for a test. You
do so and find that the meter indicates as it should. Next, you need to check for voltage among
these three wires in the cabinet. But voltage is measured between two points, so where do you
check?
A
B
C
The answer is to check between all combinations of those three points. As you can see,
the points are labeled "A", "B", and "C" in the illustration, so you would need to take your
multimeter (set in the voltmeter mode) and check between points A & B, B & C, and A & C. If
you find voltage between any of those pairs, the circuit is not in a Zero Energy State. But wait!
Remember that a multimeter will not register DC voltage when its in the AC voltage mode
and vice versa, so you need to check those three pairs of points in each mode for a total of six
voltage checks in order to be complete!
However, even with all that checking, we still haven't covered all possibilities yet. Remem-
ber that hazardous voltage can appear between a single wire and ground (in this case, the
metal frame of the cabinet would be a good ground reference point) in a power system. So, to
be perfectly safe, we not only have to check between A & B, B & C, and A & C (in both AC
and DC modes), but we also have to check between A & ground, B & ground, and C & ground
(in both AC and DC modes)! This makes for a grand total of twelve voltage checks for this
seemingly simple scenario of only three wires. Then, of course, after we've completed all these
checks, we need to take our multimeter and re-test it against a known source of voltage such
as a power receptacle to ensure that its still in good working order.
Using a multimeter to check for resistance is a much simpler task. The test leads will be
kept plugged in the same sockets as for the voltage checks, but the selector switch will need to
3.9.
SAFE METER USAGE
111
be turned until it points to the "horseshoe" resistance symbol. Touching the probes across the
device whose resistance is to be measured, the meter should properly display the resistance in
ohms:
COM
A
V
V
A
A
OFF
k
carbon-composition
resistor
One very important thing to remember about measuring resistance is that it must only be
done on de-energized components! When the meter is in "resistance" mode, it uses a small
internal battery to generate a tiny current through the component to be measured. By sensing
how difficult it is to move this current through the component, the resistance of that component
can be determined and displayed. If there is any additional source of voltage in the meter-lead-
component-lead-meter loop to either aid or oppose the resistance-measuring current produced
by the meter, faulty readings will result. In a worse-case situation, the meter may even be
damaged by the external voltage.
The "resistance" mode of a multimeter is very useful in determining wire continuity as well
as making precise measurements of resistance. When there is a good, solid connection between
the probe tips (simulated by touching them together), the meter shows almost zero Ω. If the
test leads had no resistance in them, it would read exactly zero:
COM
A
V
V
A
A
OFF
If the leads are not in contact with each other, or touching opposite ends of a broken wire,
112
CHAPTER 3. ELECTRICAL SAFETY
the meter will indicate infinite resistance (usually by displaying dashed lines or the abbrevia-
tion "O.L." which stands for "open loop"):
COM
A
V
V
A
A
OFF
By far the most hazardous and complex application of the multimeter is in the measure-
ment of current. The reason for this is quite simple: in order for the meter to measure current,
the current to be measured must be forced to go through the meter. This means that the meter
must be made part of the current path of the circuit rather than just be connected off to the
side somewhere as is the case when measuring voltage. In order to make the meter part of
the current path of the circuit, the original circuit must be "broken" and the meter connected
across the two points of the open break. To set the meter up for this, the selector switch must
point to either AC or DC "A" and the red test lead must be plugged in the red socket marked
"A". The following illustration shows a meter all ready to measure current and a circuit to be
tested:
COM
A
V
V
A
A
OFF
9
volts
simple battery-lamp circuit
Now, the circuit is broken in preparation for the meter to be connected:
3.9.
SAFE METER USAGE
113
COM
A
V
V
A
A
OFF
9
volts
lamp goes out
The next step is to insert the meter in-line with the circuit by connecting the two probe
tips to the broken ends of the circuit, the black probe to the negative (-) terminal of the 9-volt
battery and the red probe to the loose wire end leading to the lamp:
COM
A
V
V
A
A
OFF
9
volts
m
circuit current now has to
go through the meter
This example shows a very safe circuit to work with. 9 volts hardly constitutes a shock
hazard, and so there is little to fear in breaking this circuit open (bare handed, no less!) and
connecting the meter in-line with the flow of electrons. However, with higher power circuits,
this could be a hazardous endeavor indeed. Even if the circuit voltage was low, the normal
current could be high enough that an injurious spark would result the moment the last meter
probe connection was established.
Another potential hazard of using a multimeter in its current-measuring ("ammeter") mode
is failure to properly put it back into a voltage-measuring configuration before measuring volt-
age with it. The reasons for this are specific to ammeter design and operation. When mea-
suring circuit current by placing the meter directly in the path of current, it is best to have
the meter offer little or no resistance against the flow of electrons. Otherwise, any additional
114
CHAPTER 3. ELECTRICAL SAFETY
resistance offered by the meter would impede the electron flow and alter the circuits operation.
Thus, the multimeter is designed to have practically zero ohms of resistance between the test
probe tips when the red probe has been plugged into the red "A" (current-measuring) socket. In
the voltage-measuring mode (red lead plugged into the red "V" socket), there are many mega-
ohms of resistance between the test probe tips, because voltmeters are designed to have close
to infinite resistance (so that they don't draw any appreciable current from the circuit under
test).
When switching a multimeter from current- to voltage-measuring mode, its easy to spin
the selector switch from the "A" to the "V" position and forget to correspondingly switch the
position of the red test lead plug from "A" to "V". The result - if the meter is then connected
across a source of substantial voltage - will be a short-circuit through the meter!
COM
A
V
V
A
A
OFF
SHORT-CIRCUIT!
To help prevent this, most multimeters have a warning feature by which they beep if ever
there's a lead plugged in the "A" socket and the selector switch is set to "V". As convenient as
features like these are, though, they are still no substitute for clear thinking and caution when
using a multimeter.
All good-quality multimeters contain fuses inside that are engineered to "blow" in the event
of excessive current through them, such as in the case illustrated in the last image. Like all
overcurrent protection devices, these fuses are primarily designed to protect the equipment (in
this case, the meter itself) from excessive damage, and only secondarily to protect the user from
harm. A multimeter can be used to check its own current fuse by setting the selector switch to
the resistance position and creating a connection between the two red sockets like this:
3.9.
SAFE METER USAGE
115
COM
A
V
V
A
A
OFF
touch probe tips
together
COM
A
V
V
A
A
OFF
touch probe tips
together
Indication with a good fuse
Indication with a "blown" fuse
A good fuse will indicate very little resistance while a blown fuse will always show "O.L."
(or whatever indication that model of multimeter uses to indicate no continuity). The actual
number of ohms displayed for a good fuse is of little consequence, so long as its an arbitrarily
low figure.
So now that we've seen how to use a multimeter to measure voltage, resistance, and current,
what more is there to know? Plenty! The value and capabilities of this versatile test instrument
will become more evident as you gain skill and familiarity using it. There is no substitute for
regular practice with complex instruments such as these, so feel free to experiment on safe,
battery-powered circuits.
• REVIEW:
• A meter capable of checking for voltage, current, and resistance is called a multimeter.
• As voltage is always relative between two points, a voltage-measuring meter ("voltmeter")
must be connected to two points in a circuit in order to obtain a good reading. Be careful
not to touch the bare probe tips together while measuring voltage, as this will create a
short-circuit!
• Remember to always check for both AC and DC voltage when using a multimeter to
check for the presence of hazardous voltage on a circuit. Make sure you check for voltage
between all pair-combinations of conductors, including between the individual conductors
and ground!
• When in the voltage-measuring ("voltmeter") mode, multimeters have very high resis-
tance between their leads.
• Never try to read resistance or continuity with a multimeter on a circuit that is energized.
At best, the resistance readings you obtain from the meter will be inaccurate, and at worst
the meter may be damaged and you may be injured.
• Current measuring meters ("ammeters") are always connected in a circuit so the electrons
have to flow through the meter.
116
CHAPTER 3. ELECTRICAL SAFETY
• When in the current-measuring ("ammeter") mode, multimeters have practically no resis-
tance between their leads. This is intended to allow electrons to flow through the meter
with the least possible difficulty. If this were not the case, the meter would add extra
resistance in the circuit, thereby affecting the current.
3.10
Electric shock data
The table of electric currents and their various bodily effects was obtained from online (Inter-
net) sources: the safety page of Massachusetts Institute of Technology (website: (https://ehs.mit.edu/workplace
and a safety handbook published by Cooper Bussmann, Inc (website: (http://www.bussmann.com)).
In the Bussmann handbook, the table is appropriately entitled Deleterious Effects of Electric
Shock, and credited to a Mr. Charles F. Dalziel. Further research revealed Dalziel to be both a
scientific pioneer and an authority on the effects of electricity on the human body.
The table found in the Bussmann handbook differs slightly from the one available from
MIT: for the DC threshold of perception (men), the MIT table gives 5.2 mA while the Bussmann
table gives a slightly greater figure of 6.2 mA. Also, for the "unable to let go" 60 Hz AC threshold
(men), the MIT table gives 20 mA while the Bussmann table gives a lesser figure of 16 mA. As I
have yet to obtain a primary copy of Dalziel's research, the figures cited here are conservative:
I have listed the lowest values in my table where any data sources differ.
These differences, of course, are academic. The point here is that relatively small magni-
tudes of electric current through the body can be harmful if not lethal.
Data regarding the electrical resistance of body contact points was taken from a safety page
(document 16.1) from the Lawrence Livermore National Laboratory (website (http://www-ais.llnl.gov)),
citing Ralph H. Lee as the data source. Lee's work was listed here in a document entitled "Hu-
man Electrical Sheet," composed while he was an IEEE Fellow at E.I. duPont de Nemours &
Co., and also in an article entitled "Electrical Safety in Industrial Plants" found in the June
1971 issue of IEEE Spectrum magazine.
For the morbidly curious, Charles Dalziel's experimentation conducted at the University
of California (Berkeley) began with a state grant to investigate the bodily effects of sub-lethal
electric current. His testing method was as follows: healthy male and female volunteer subjects
were asked to hold a copper wire in one hand and place their other hand on a round, brass
plate. A voltage was then applied between the wire and the plate, causing electrons to flow
through the subject's arms and chest. The current was stopped, then resumed at a higher
level. The goal here was to see how much current the subject could tolerate and still keep their
hand pressed against the brass plate. When this threshold was reached, laboratory assistants
forcefully held the subject's hand in contact with the plate and the current was again increased.
The subject was asked to release the wire they were holding, to see at what current level
involuntary muscle contraction (tetanus) prevented them from doing so. For each subject the
experiment was conducted using DC and also AC at various frequencies.
Over two dozen
human volunteers were tested, and later studies on heart fibrillation were conducted using
animal subjects.
3.11.
CONTRIBUTORS
117
3.11
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Bibliography
[1] Robert S. Porter, MD, editor, "The Merck Manuals Online Medical Library", "Electrical
Injuries," at http://www.merck.com/mmpe/sec21/ch316/ch316b.html
118
CHAPTER 3. ELECTRICAL SAFETY
Chapter 4
SCIENTIFIC NOTATION AND
METRIC PREFIXES
Contents
4.1
Scientific notation
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 119
4.2
Arithmetic with scientific notation . . . . . . . . . . . . . . . . . . . . . . . . 121
4.3
Metric notation . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 123
4.4
Metric prefix conversions . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 124
4.5
Hand calculator use . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 125
4.6
Scientific notation in SPICE . . . . . . . . . . . . . . . . . . . . . . . . . . . . 126
4.7
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 128
4.1
Scientific notation
In many disciplines of science and engineering, very large and very small numerical quantities
must be managed. Some of these quantities are mind-boggling in their size, either extremely
small or extremely large. Take for example the mass of a proton, one of the constituent particles
of an atom's nucleus:
Proton mass = 0.00000000000000000000000167 grams
Or, consider the number of electrons passing by a point in a circuit every second with a
steady electric current of 1 amp:
1 amp = 6,250,000,000,000,000,000 electrons per second
A lot of zeros, isn't it? Obviously, it can get quite confusing to have to handle so many zero
digits in numbers such as this, even with the help of calculators and computers.
119
120
CHAPTER 4. SCIENTIFIC NOTATION AND METRIC PREFIXES
Take note of those two numbers and of the relative sparsity of non-zero digits in them. For
the mass of the proton, all we have is a "167" preceded by 23 zeros before the decimal point. For
the number of electrons per second in 1 amp, we have "625" followed by 16 zeros. We call the
span of non-zero digits (from first to last), plus any zero digits not merely used for placeholding,
the "significant digits" of any number.
The significant digits in a real-world measurement are typically reflective of the accuracy of
that measurement. For example, if we were to say that a car weighs 3,000 pounds, we probably
don't mean that the car in question weighs exactly 3,000 pounds, but that we've rounded its
weight to a value more convenient to say and remember. That rounded figure of 3,000 has only
one significant digit: the "3" in front - the zeros merely serve as placeholders. However, if we
were to say that the car weighed 3,005 pounds, the fact that the weight is not rounded to the
nearest thousand pounds tells us that the two zeros in the middle aren't just placeholders, but
that all four digits of the number "3,005" are significant to its representative accuracy. Thus,
the number "3,005" is said to have four significant figures.
In like manner, numbers with many zero digits are not necessarily representative of a real-
world quantity all the way to the decimal point. When this is known to be the case, such a
number can be written in a kind of mathematical "shorthand" to make it easier to deal with.
This "shorthand" is called scientific notation.
With scientific notation, a number is written by representing its significant digits as a
quantity between 1 and 10 (or -1 and -10, for negative numbers), and the "placeholder" zeros
are accounted for by a power-of-ten multiplier. For example:
1 amp = 6,250,000,000,000,000,000 electrons per second
. . . can be expressed as . . .
1 amp = 6.25 x 1018 electrons per second
10 to the 18th power (1018) means 10 multiplied by itself 18 times, or a "1" followed by 18
zeros. Multiplied by 6.25, it looks like "625" followed by 16 zeros (take 6.25 and skip the decimal
point 18 places to the right). The advantages of scientific notation are obvious: the number isn't
as unwieldy when written on paper, and the significant digits are plain to identify.
But what about very small numbers, like the mass of the proton in grams? We can still use
scientific notation, except with a negative power-of-ten instead of a positive one, to shift the
decimal point to the left instead of to the right:
Proton mass = 0.00000000000000000000000167 grams
. . . can be expressed as . . .
Proton mass = 1.67 x 10−24 grams
10 to the -24th power (10−24) means the inverse (1/x) of 10 multiplied by itself 24 times, or
a "1" preceded by a decimal point and 23 zeros. Multiplied by 1.67, it looks like "167" preceded
by a decimal point and 23 zeros. Just as in the case with the very large number, it is a lot
4.2.
ARITHMETIC WITH SCIENTIFIC NOTATION
121
easier for a human being to deal with this "shorthand" notation. As with the prior case, the
significant digits in this quantity are clearly expressed.
Because the significant digits are represented "on their own," away from the power-of-ten
multiplier, it is easy to show a level of precision even when the number looks round. Taking our
3,000 pound car example, we could express the rounded number of 3,000 in scientific notation
as such:
car weight = 3 x 103 pounds
If the car actually weighed 3,005 pounds (accurate to the nearest pound) and we wanted
to be able to express that full accuracy of measurement, the scientific notation figure could be
written like this:
car weight = 3.005 x 103 pounds
However, what if the car actually did weigh 3,000 pounds, exactly (to the nearest pound)?
If we were to write its weight in "normal" form (3,000 lbs), it wouldn't necessarily be clear that
this number was indeed accurate to the nearest pound and not just rounded to the nearest
thousand pounds, or to the nearest hundred pounds, or to the nearest ten pounds. Scientific
notation, on the other hand, allows us to show that all four digits are significant with no
misunderstanding:
car weight = 3.000 x 103 pounds
Since there would be no point in adding extra zeros to the right of the decimal point (place-
holding zeros being unnecessary with scientific notation), we know those zeros must be signif-
icant to the precision of the figure.
4.2
Arithmetic with scientific notation
The benefits of scientific notation do not end with ease of writing and expression of accuracy.
Such notation also lends itself well to mathematical problems of multiplication and division.
Let's say we wanted to know how many electrons would flow past a point in a circuit carrying
1 amp of electric current in 25 seconds. If we know the number of electrons per second in the
circuit (which we do), then all we need to do is multiply that quantity by the number of seconds
(25) to arrive at an answer of total electrons:
(6,250,000,000,000,000,000 electrons per second) x (25 seconds) =
156,250,000,000,000,000,000 electrons passing by in 25 seconds
Using scientific notation, we can write the problem like this:
(6.25 x 1018 electrons per second) x (25 seconds)
If we take the "6.25" and multiply it by 25, we get 156.25. So, the answer could be written
as:
122
CHAPTER 4. SCIENTIFIC NOTATION AND METRIC PREFIXES
156.25 x 1018 electrons
However, if we want to hold to standard convention for scientific notation, we must rep-
resent the significant digits as a number between 1 and 10. In this case, we'd say "1.5625"
multiplied by some power-of-ten. To obtain 1.5625 from 156.25, we have to skip the decimal
point two places to the left. To compensate for this without changing the value of the number,
we have to raise our power by two notches (10 to the 20th power instead of 10 to the 18th):
1.5625 x 1020 electrons
What if we wanted to see how many electrons would pass by in 3,600 seconds (1 hour)? To
make our job easier, we could put the time in scientific notation as well:
(6.25 x 1018 electrons per second) x (3.6 x 103 seconds)
To multiply, we must take the two significant sets of digits (6.25 and 3.6) and multiply
them together; and we need to take the two powers-of-ten and multiply them together. Taking
6.25 times 3.6, we get 22.5. Taking 1018 times 103, we get 1021 (exponents with common base
numbers add). So, the answer is:
22.5 x 1021 electrons
. . . or more properly . . .
2.25 x 1022 electrons
To illustrate how division works with scientific notation, we could figure that last problem
"backwards" to find out how long it would take for that many electrons to pass by at a current
of 1 amp:
(2.25 x 1022 electrons) / (6.25 x 1018 electrons per second)
Just as in multiplication, we can handle the significant digits and powers-of-ten in separate
steps (remember that you subtract the exponents of divided powers-of-ten):
(2.25 / 6.25) x (1022 / 1018)
And the answer is: 0.36 x 104, or 3.6 x 103, seconds. You can see that we arrived at the
same quantity of time (3600 seconds). Now, you may be wondering what the point of all this
is when we have electronic calculators that can handle the math automatically. Well, back
in the days of scientists and engineers using "slide rule" analog computers, these techniques
were indispensable. The "hard" arithmetic (dealing with the significant digit figures) would be
performed with the slide rule while the powers-of-ten could be figured without any help at all,
being nothing more than simple addition and subtraction.
• REVIEW:
4.3.
METRIC NOTATION
123
• Significant digits are representative of the real-world accuracy of a number.
• Scientific notation is a "shorthand" method to represent very large and very small num-
bers in easily-handled form.
• When multiplying two numbers in scientific notation, you can multiply the two significant
digit figures and arrive at a power-of-ten by adding exponents.
• When dividing two numbers in scientific notation, you can divide the two significant digit
figures and arrive at a power-of-ten by subtracting exponents.
4.3
Metric notation
The metric system, besides being a collection of measurement units for all sorts of physical
quantities, is structured around the concept of scientific notation. The primary difference is
that the powers-of-ten are represented with alphabetical prefixes instead of by literal powers-
of-ten. The following number line shows some of the more common prefixes and their respective
100
103
106
109
1012
(none)
kilo
mega
giga
tera
milli
micro
nano
pico
k
M
G
T
m
μ
n
p
101
102
deci
centi
deca
hecto
h
da
d
c
METRIC PREFIX SCALE
Looking at this scale, we can see that 2.5 Gigabytes would mean 2.5 x 109 bytes, or 2.5
billion bytes. Likewise, 3.21 picoamps would mean 3.21 x 10−12 amps, or 3.21 1/trillionths of
an amp.
Other metric prefixes exist to symbolize powers of ten for extremely small and extremely
large multipliers. On the extremely small end of the spectrum, femto (f) = 10−15, atto (a) =
10−18, zepto (z) = 10−21, and yocto (y) = 10−24. On the extremely large end of the spectrum,
Peta (P) = 1015, Exa (E) = 1018, Zetta (Z) = 1021, and Yotta (Y) = 1024.
Because the major prefixes in the metric system refer to powers of 10 that are multiples of
3 (from "kilo" on up, and from "milli" on down), metric notation differs from regular scientific
notation in that the mantissa can be anywhere between 1 and 999, depending on which prefix
is chosen. For example, if a laboratory sample weighs 0.000267 grams, scientific notation and
metric notation would express it differently:
2.67 x 10−4 grams (scientific notation)
267 μgrams (metric notation)
The same figure may also be expressed as 0.267 milligrams (0.267 mg), although it is usu-
ally more common to see the significant digits represented as a figure greater than 1.
124
CHAPTER 4. SCIENTIFIC NOTATION AND METRIC PREFIXES
In recent years a new style of metric notation for electric quantities has emerged which
seeks to avoid the use of the decimal point. Since decimal points (".") are easily misread and/or
"lost" due to poor print quality, quantities such as 4.7 k may be mistaken for 47 k. The new
notation replaces the decimal point with the metric prefix character, so that "4.7 k" is printed
instead as "4k7". Our last figure from the prior example, "0.267 m", would be expressed in the
new notation as "0m267".
• REVIEW:
• The metric system of notation uses alphabetical prefixes to represent certain powers-of-
ten instead of the lengthier scientific notation.
4.4
Metric prefix conversions
To express a quantity in a different metric prefix that what it was originally given, all we need
to do is skip the decimal point to the right or to the left as needed. Notice that the metric prefix
"number line" in the previous section was laid out from larger to smaller, left to right. This
layout was purposely chosen to make it easier to remember which direction you need to skip
the decimal point for any given conversion.
Example problem: express 0.000023 amps in terms of microamps.
0.000023 amps (has no prefix, just plain unit of amps)
From UNITS to micro on the number line is 6 places (powers of ten) to the right, so we need
to skip the decimal point 6 places to the right:
0.000023 amps = 23. , or 23 microamps (μA)
Example problem: express 304,212 volts in terms of kilovolts.
304,212 volts (has no prefix, just plain unit of volts)
From the (none) place to kilo place on the number line is 3 places (powers of ten) to the left,
so we need to skip the decimal point 3 places to the left:
304,212. = 304.212 kilovolts (kV)
Example problem: express 50.3 Mega-ohms in terms of milli-ohms.
50.3 M ohms (mega = 106)
From mega to milli is 9 places (powers of ten) to the right (from 10 to the 6th power to 10
to the -3rd power), so we need to skip the decimal point 9 places to the right:
50.3 M ohms = 50,300,000,000 milli-ohms (mΩ)
4.5.
HAND CALCULATOR USE
125
• REVIEW:
• Follow the metric prefix number line to know which direction you skip the decimal point
for conversion purposes.
• A number with no decimal point shown has an implicit decimal point to the immediate
right of the furthest right digit (i.e. for the number 436 the decimal point is to the right
of the 6, as such: 436.)
4.5
Hand calculator use
To enter numbers in scientific notation into a hand calculator, there is usually a button marked
"E" or "EE" used to enter the correct power of ten. For example, to enter the mass of a proton
in grams (1.67 x 10−24 grams) into a hand calculator, I would enter the following keystrokes:
The [+/-] keystroke changes the sign of the power (24) into a -24. Some calculators allow
the use of the subtraction key [-] to do this, but I prefer the "change sign" [+/-] key because its
more consistent with the use of that key in other contexts.
If I wanted to enter a negative number in scientific notation into a hand calculator, I would
have to be careful how I used the [+/-] key, lest I change the sign of the power and not the
significant digit value. Pay attention to this example:
Number to be entered: -3.221 x 10−15:
changes the power from 15 to -15.
Displaying metric and scientific notation on a hand calculator is a different matter.
It
involves changing the display option from the normal "fixed" decimal point mode to the "sci-
entific" or "engineering" mode. Your calculator manual will tell you how to set each display
mode.
These display modes tell the calculator how to represent any number on the numerical
readout. The actual value of the number is not affected in any way by the choice of display
modes - only how the number appears to the calculator user. Likewise, the procedure for en-
tering numbers into the calculator does not change with different display modes either. Powers
of ten are usually represented by a pair of digits in the upper-right hand corner of the display,
and are visible only in the "scientific" and "engineering" modes.
The difference between "scientific" and "engineering" display modes is the difference be-
tween scientific and metric notation. In "scientific" mode, the power-of-ten display is set so
that the main number on the display is always a value between 1 and 10 (or -1 and -10 for
negative numbers). In "engineering" mode, the powers-of-ten are set to display in multiples of
3, to represent the major metric prefixes. All the user has to do is memorize a few prefix/power
combinations, and his or her calculator will be "speaking" metric!
126
CHAPTER 4. SCIENTIFIC NOTATION AND METRIC PREFIXES
POWER
METRIC PREFIX
12 ......... Tera
(T)
9 .......... Giga
(G)
6 .......... Mega
(M)
3 .......... Kilo
(k)
0 .......... UNITS (plain)
-3 ......... milli (m)
-6 ......... micro (u)
(n)
(p)
• REVIEW:
• Use the [EE] key to enter powers of ten.
• Use "scientific" or "engineering" to display powers of ten, in scientific or metric notation,
respectively.
4.6
Scientific notation in SPICE
The SPICE circuit simulation computer program uses scientific notation to display its output
information, and can interpret both scientific notation and metric prefixes in the circuit de-
scription files. If you are going to be able to successfully interpret the SPICE analyses through-
out this book, you must be able to understand the notation used to express variables of voltage,
current, etc. in the program.
Let's start with a very simple circuit composed of one voltage source (a battery) and one
resistor:
24 V
5 Ω
To simulate this circuit using SPICE, we first have to designate node numbers for all the
distinct points in the circuit, then list the components along with their respective node num-
bers so the computer knows which component is connected to which, and how. For a circuit of
this simplicity, the use of SPICE seems like overkill, but it serves the purpose of demonstrating
practical use of scientific notation:
4.6.
SCIENTIFIC NOTATION IN SPICE
127
24 V
1
1
0
0
5 Ω
Typing out a circuit description file, or netlist, for this circuit, we get this:
simple circuit
v1 1 0 dc 24
r1 1 0 5
.end
The line "v1 1 0 dc 24" describes the battery, positioned between nodes 1 and 0, with a
DC voltage of 24 volts. The line "r1 1 0 5" describes the 5 Ωresistor placed between nodes 1
and 0.
Using a computer to run a SPICE analysis on this circuit description file, we get the follow-
ing results:
node
voltage
1)
24.0000
voltage source currents
name
current
v1
total power dissipation
watts
SPICE tells us that the voltage "at" node number 1 (actually, this means the voltage be-
tween nodes 1 and 0, node 0 being the default reference point for all voltage measurements) is
equal to 24 volts. The current through battery "v1" is displayed as -4.800E+00 amps. This is
SPICE's method of denoting scientific notation. What its really saying is "-4.800 x 100 amps,"
or simply -4.800 amps. The negative value for current here is due to a quirk in SPICE and does
not indicate anything significant about the circuit itself. The "total power dissipation" is given
to us as 1.15E+02 watts, which means "1.15 x 102 watts," or 115 watts.
Let's modify our example circuit so that it has a 5 kΩ(5 kilo-ohm, or 5,000 ohm) resistor
instead of a 5 Ωresistor and see what happens.
24 V
1
1
0
0
5 kΩ
128
CHAPTER 4. SCIENTIFIC NOTATION AND METRIC PREFIXES
Once again is our circuit description file, or "netlist:"
simple circuit
v1 1 0 dc 24
r1 1 0 5k
.end
The letter "k" following the number 5 on the resistor's line tells SPICE that it is a figure of
5 kΩ, not 5 Ω. Let's see what result we get when we run this through the computer:
node
voltage
1)
24.0000
voltage source currents
name
current
v1
total power dissipation
watts
The battery voltage, of course, hasn't changed since the first simulation: its still at 24 volts.
The circuit current, on the other hand, is much less this time because we've made the resistor
a larger value, making it more difficult for electrons to flow. SPICE tells us that the current
this time is equal to -4.800E-03 amps, or -4.800 x 10−3 amps. This is equivalent to taking the
number -4.8 and skipping the decimal point three places to the left.
Of course, if we recognize that 10−3 is the same as the metric prefix "milli," we could write
the figure as -4.8 milliamps, or -4.8 mA.
Looking at the "total power dissipation" given to us by SPICE on this second simulation, we
see that it is 1.15E-01 watts, or 1.15 x 10−1 watts. The power of -1 corresponds to the metric
prefix "deci," but generally we limit our use of metric prefixes in electronics to those associated
with powers of ten that are multiples of three (ten to the power of . . . -12, -9, -6, -3, 3, 6, 9, 12,
etc.). So, if we want to follow this convention, we must express this power dissipation figure as
0.115 watts or 115 milliwatts (115 mW) rather than 1.15 deciwatts (1.15 dW).
Perhaps the easiest way to convert a figure from scientific notation to common metric pre-
fixes is with a scientific calculator set to the "engineering" or "metric" display mode. Just set
the calculator for that display mode, type any scientific notation figure into it using the proper
keystrokes (see your owner's manual), press the "equals" or "enter" key, and it should display
the same figure in engineering/metric notation.
Again, I'll be using SPICE as a method of demonstrating circuit concepts throughout this
book. Consequently, it is in your best interest to understand scientific notation so you can
easily comprehend its output data format.
4.7
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 5
SERIES AND PARALLEL
CIRCUITS
Contents
5.1
What are "series" and "parallel" circuits?
. . . . . . . . . . . . . . . . . . . 129
5.2
Simple series circuits
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 132
5.3
Simple parallel circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 139
5.4
Conductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 144
5.5
Power calculations . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 146
5.6
Correct use of Ohm's Law . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 147
5.7
Component failure analysis
. . . . . . . . . . . . . . . . . . . . . . . . . . . . 149
5.8
Building simple resistor circuits . . . . . . . . . . . . . . . . . . . . . . . . . 155
5.9
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 170
5.1
What are "series" and "parallel" circuits?
Circuits consisting of just one battery and one load resistance are very simple to analyze, but
they are not often found in practical applications. Usually, we find circuits where more than
two components are connected together.
There are two basic ways in which to connect more than two circuit components: series and
parallel. First, an example of a series circuit:
129
130
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
1
2
3
4
R1
R2
R3
Series
Here, we have three resistors (labeled R1, R2, and R3), connected in a long chain from one
terminal of the battery to the other. (It should be noted that the subscript labeling - those
little numbers to the lower-right of the letter "R" - are unrelated to the resistor values in
ohms. They serve only to identify one resistor from another.) The defining characteristic of a
series circuit is that there is only one path for electrons to flow. In this circuit the electrons flow
in a counter-clockwise direction, from point 4 to point 3 to point 2 to point 1 and back around
to 4.
Now, let's look at the other type of circuit, a parallel configuration:
1
2
3
4
5
6
7
8
R1
R2
R3
Parallel
Again, we have three resistors, but this time they form more than one continuous path for
electrons to flow. There's one path from 8 to 7 to 2 to 1 and back to 8 again. There's another
from 8 to 7 to 6 to 3 to 2 to 1 and back to 8 again. And then there's a third path from 8 to 7 to
6 to 5 to 4 to 3 to 2 to 1 and back to 8 again. Each individual path (through R1, R2, and R3) is
called a branch.
The defining characteristic of a parallel circuit is that all components are connected between
the same set of electrically common points. Looking at the schematic diagram, we see that
points 1, 2, 3, and 4 are all electrically common. So are points 8, 7, 6, and 5. Note that all
resistors as well as the battery are connected between these two sets of points.
And, of course, the complexity doesn't stop at simple series and parallel either! We can have
circuits that are a combination of series and parallel, too:
5.1.
WHAT ARE "SERIES" AND "PARALLEL" CIRCUITS?
131
1
2
3
4
5
6
R1
R2
R3
Series-parallel
In this circuit, we have two loops for electrons to flow through: one from 6 to 5 to 2 to 1 and
back to 6 again, and another from 6 to 5 to 4 to 3 to 2 to 1 and back to 6 again. Notice how both
current paths go through R1 (from point 2 to point 1). In this configuration, we'd say that R2
and R3 are in parallel with each other, while R1 is in series with the parallel combination of R2
and R3.
This is just a preview of things to come. Don't worry! We'll explore all these circuit configu-
rations in detail, one at a time!
The basic idea of a "series" connection is that components are connected end-to-end in a line
to form a single path for electrons to flow:
only one path for electrons to flow!
R1
R2
R3
R4
Series connection
The basic idea of a "parallel" connection, on the other hand, is that all components are
connected across each other's leads. In a purely parallel circuit, there are never more than two
sets of electrically common points, no matter how many components are connected. There are
many paths for electrons to flow, but only one voltage across all components:
132
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
These points are electrically common
These points are electrically common
R1
R2
R3
R4
Parallel connection
Series and parallel resistor configurations have very different electrical properties. We'll
explore the properties of each configuration in the sections to come.
• REVIEW:
• In a series circuit, all components are connected end-to-end, forming a single path for
electrons to flow.
• In a parallel circuit, all components are connected across each other, forming exactly two
sets of electrically common points.
• A "branch" in a parallel circuit is a path for electric current formed by one of the load
components (such as a resistor).
5.2
Simple series circuits
Let's start with a series circuit consisting of three resistors and a single battery:
1
2
3
4
9 V
R1
R2
R3
3 kΩ
10 kΩ
5 kΩ
The first principle to understand about series circuits is that the amount of current is the
same through any component in the circuit. This is because there is only one path for electrons
to flow in a series circuit, and because free electrons flow through conductors like marbles in a
tube, the rate of flow (marble speed) at any point in the circuit (tube) at any specific point in
time must be equal.
5.2.
SIMPLE SERIES CIRCUITS
133
From the way that the 9 volt battery is arranged, we can tell that the electrons in this circuit
will flow in a counter-clockwise direction, from point 4 to 3 to 2 to 1 and back to 4. However,
we have one source of voltage and three resistances. How do we use Ohm's Law here?
An important caveat to Ohm's Law is that all quantities (voltage, current, resistance, and
power) must relate to each other in terms of the same two points in a circuit. For instance, with
a single-battery, single-resistor circuit, we could easily calculate any quantity because they all
applied to the same two points in the circuit:
1
2
3
4
9 V
3 kΩ
R
I
9 volts
3 kΩ
Since points 1 and 2 are connected together with wire of negligible resistance, as are points
3 and 4, we can say that point 1 is electrically common to point 2, and that point 3 is electrically
common to point 4. Since we know we have 9 volts of electromotive force between points 1 and
4 (directly across the battery), and since point 2 is common to point 1 and point 3 common
to point 4, we must also have 9 volts between points 2 and 3 (directly across the resistor).
Therefore, we can apply Ohm's Law (I = E/R) to the current through the resistor, because we
know the voltage (E) across the resistor and the resistance (R) of that resistor. All terms (E, I,
R) apply to the same two points in the circuit, to that same resistor, so we can use the Ohm's
Law formula with no reservation.
However, in circuits containing more than one resistor, we must be careful in how we apply
Ohm's Law. In the three-resistor example circuit below, we know that we have 9 volts between
points 1 and 4, which is the amount of electromotive force trying to push electrons through the
series combination of R1, R2, and R3. However, we cannot take the value of 9 volts and divide
it by 3k, 10k or 5k Ωto try to find a current value, because we don't know how much voltage is
across any one of those resistors, individually.
134
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
1
2
3
4
9 V
R1
R2
R3
3 kΩ
10 kΩ
5 kΩ
The figure of 9 volts is a total quantity for the whole circuit, whereas the figures of 3k, 10k,
and 5k Ωare individual quantities for individual resistors. If we were to plug a figure for total
voltage into an Ohm's Law equation with a figure for individual resistance, the result would
not relate accurately to any quantity in the real circuit.
For R1, Ohm's Law will relate the amount of voltage across R1 with the current through R1,
given R1's resistance, 3kΩ:
ER1
3 kΩ
ER1 = IR1(3 kΩ)
But, since we don't know the voltage across R1 (only the total voltage supplied by the battery
across the three-resistor series combination) and we don't know the current through R1, we
can't do any calculations with either formula. The same goes for R2 and R3: we can apply the
Ohm's Law equations if and only if all terms are representative of their respective quantities
between the same two points in the circuit.
So what can we do? We know the voltage of the source (9 volts) applied across the series
combination of R1, R2, and R3, and we know the resistances of each resistor, but since those
quantities aren't in the same context, we can't use Ohm's Law to determine the circuit current.
If only we knew what the total resistance was for the circuit: then we could calculate total
current with our figure for total voltage (I=E/R).
This brings us to the second principle of series circuits: the total resistance of any series
circuit is equal to the sum of the individual resistances. This should make intuitive sense: the
more resistors in series that the electrons must flow through, the more difficult it will be for
those electrons to flow. In the example problem, we had a 3 kΩ, 10 kΩ, and 5 kΩresistor in
series, giving us a total resistance of 18 kΩ:
Rtotal = R1
R2
R3
Rtotal = 3 kΩ
10 kΩ
5 kΩ
Rtotal = 18 kΩ
In essence, we've calculated the equivalent resistance of R1, R2, and R3 combined. Knowing
this, we could re-draw the circuit with a single equivalent resistor representing the series
combination of R1, R2, and R3:
5.2.
SIMPLE SERIES CIRCUITS
135
1
4
18 kΩ
9 V
Now we have all the necessary information to calculate circuit current, because we have
the voltage between points 1 and 4 (9 volts) and the resistance between points 1 and 4 (18 kΩ):
9 volts
18 kΩ
500 μA
Itotal
Etotal
Rtotal
Knowing that current is equal through all components of a series circuit (and we just deter-
mined the current through the battery), we can go back to our original circuit schematic and
note the current through each component:
1
2
3
4
9 V
R1
R2
R3
3 kΩ
10 kΩ
5 kΩ
Now that we know the amount of current through each resistor, we can use Ohm's Law to
determine the voltage drop across each one (applying Ohm's Law in its proper context):
ER1 = IR1 R1
ER2 = IR2 R2
ER3 = IR3 R3
ER1 = (500 μA)(3 kΩ) = 1.5 V
ER2 = (500 μA)(10 kΩ) = 5 V
ER3 = (500 μA)(5 kΩ) = 2.5 V
Notice the voltage drops across each resistor, and how the sum of the voltage drops (1.5 +
5 + 2.5) is equal to the battery (supply) voltage: 9 volts. This is the third principle of series
circuits: that the supply voltage is equal to the sum of the individual voltage drops.
However, the method we just used to analyze this simple series circuit can be streamlined
for better understanding. By using a table to list all voltages, currents, and resistances in the
136
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
circuit, it becomes very easy to see which of those quantities can be properly related in any
Ohm's Law equation:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
Ohm's
Law
Ohm's
Law
Ohm's
Law
Ohm's
Law
The rule with such a table is to apply Ohm's Law only to the values within each vertical
column. For instance, ER1 only with IR1 and R1; ER2 only with IR2 and R2; etc. You begin your
analysis by filling in those elements of the table that are given to you from the beginning:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
3k
10k
5k
As you can see from the arrangement of the data, we can't apply the 9 volts of ET (total
voltage) to any of the resistances (R1, R2, or R3) in any Ohm's Law formula because they're in
different columns. The 9 volts of battery voltage is not applied directly across R1, R2, or R3.
However, we can use our "rules" of series circuits to fill in blank spots on a horizontal row. In
this case, we can use the series rule of resistances to determine a total resistance from the sum
of individual resistances:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
3k
10k
5k
18k
Rule of series
circuits
Now, with a value for total resistance inserted into the rightmost ("Total") column, we can
apply Ohm's Law of I=E/R to total voltage and total resistance to arrive at a total current of
500 μA:
5.2.
SIMPLE SERIES CIRCUITS
137
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
Ohm's
Law
3k
10k
5k
18k
9
500μ
Then, knowing that the current is shared equally by all components of a series circuit
(another "rule" of series circuits), we can fill in the currents for each resistor from the current
figure just calculated:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
3k
10k
5k
18k
9
500μ
500μ
500μ
500μ
Rule of series
circuits
Finally, we can use Ohm's Law to determine the voltage drop across each resistor, one
column at a time:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
3k
10k
5k
18k
9
500μ
500μ
500μ
500μ
Ohm's
Law
Ohm's
Law
Ohm's
Law
1.5
5
2.5
Just for fun, we can use a computer to analyze this very same circuit automatically. It will
be a good way to verify our calculations and also become more familiar with computer analysis.
First, we have to describe the circuit to the computer in a format recognizable by the software.
The SPICE program we'll be using requires that all electrically unique points in a circuit be
numbered, and component placement is understood by which of those numbered points, or
"nodes," they share. For clarity, I numbered the four corners of our example circuit 1 through
4. SPICE, however, demands that there be a node zero somewhere in the circuit, so I'll re-draw
the circuit, changing the numbering scheme slightly:
138
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
1
2
3
9 V
0
R1
R2
R3
3 kΩ
10 kΩ
5 kΩ
All I've done here is re-numbered the lower-left corner of the circuit 0 instead of 4. Now,
I can enter several lines of text into a computer file describing the circuit in terms SPICE
will understand, complete with a couple of extra lines of code directing the program to display
voltage and current data for our viewing pleasure. This computer file is known as the netlist
in SPICE terminology:
series circuit
v1 1 0
r1 1 2 3k
r2 2 3 10k
r3 3 0 5k
.dc v1 9 9 1
.print dc v(1,2) v(2,3) v(3,0)
.end
Now, all I have to do is run the SPICE program to process the netlist and output the results:
v1
v(1,2)
v(2,3)
v(3)
i(v1)
9.000E+00
1.500E+00
5.000E+00
2.500E+00
This printout is telling us the battery voltage is 9 volts, and the voltage drops across R1, R2,
and R3 are 1.5 volts, 5 volts, and 2.5 volts, respectively. Voltage drops across any component in
SPICE are referenced by the node numbers the component lies between, so v(1,2) is referencing
the voltage between nodes 1 and 2 in the circuit, which are the points between which R1 is
located. The order of node numbers is important: when SPICE outputs a figure for v(1,2), it
regards the polarity the same way as if we were holding a voltmeter with the red test lead on
node 1 and the black test lead on node 2.
We also have a display showing current (albeit with a negative value) at 0.5 milliamps,
or 500 microamps. So our mathematical analysis has been vindicated by the computer. This
figure appears as a negative number in the SPICE analysis, due to a quirk in the way SPICE
handles current calculations.
In summary, a series circuit is defined as having only one path for electrons to flow. From
this definition, three rules of series circuits follow: all components share the same current;
resistances add to equal a larger, total resistance; and voltage drops add to equal a larger, total
voltage. All of these rules find root in the definition of a series circuit. If you understand that
definition fully, then the rules are nothing more than footnotes to the definition.
5.3.
SIMPLE PARALLEL CIRCUITS
139
• REVIEW:
• Components in a series circuit share the same current: IT otal = I1 = I2 = . . . In
• Total resistance in a series circuit is equal to the sum of the individual resistances: RT otal
• Total voltage in a series circuit is equal to the sum of the individual voltage drops: ET otal
5.3
Simple parallel circuits
Let's start with a parallel circuit consisting of three resistors and a single battery:
1
2
3
4
5
6
7
8
R1
R2
R3
10 kΩ
2 kΩ
1 kΩ
9 V
The first principle to understand about parallel circuits is that the voltage is equal across
all components in the circuit. This is because there are only two sets of electrically common
points in a parallel circuit, and voltage measured between sets of common points must always
be the same at any given time. Therefore, in the above circuit, the voltage across R1 is equal
to the voltage across R2 which is equal to the voltage across R3 which is equal to the voltage
across the battery. This equality of voltages can be represented in another table for our starting
values:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
10k
2k
1k
Just as in the case of series circuits, the same caveat for Ohm's Law applies: values for
voltage, current, and resistance must be in the same context in order for the calculations to
work correctly. However, in the above example circuit, we can immediately apply Ohm's Law
to each resistor to find its current because we know the voltage across each resistor (9 volts)
and the resistance of each resistor:
140
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
ER1
R1
ER2
R2
ER3
R3
9 V
10 kΩ
9 V
2 kΩ
4.5 mA
9 V
1 kΩ
9 mA
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
10k
2k
1k
0.9m
4.5m
9m
Ohm's
Law
Ohm's
Law
Ohm's
Law
At this point we still don't know what the total current or total resistance for this parallel
circuit is, so we can't apply Ohm's Law to the rightmost ("Total") column. However, if we think
carefully about what is happening it should become apparent that the total current must equal
the sum of all individual resistor ("branch") currents:
1
2
3
4
5
6
7
8
IT
IT
R1
R2
R3
10 kΩ
2 kΩ
1 kΩ
9 V
IR1
IR2
IR3
As the total current exits the negative (-) battery terminal at point 8 and travels through
the circuit, some of the flow splits off at point 7 to go up through R1, some more splits off at
point 6 to go up through R2, and the remainder goes up through R3. Like a river branching
into several smaller streams, the combined flow rates of all streams must equal the flow rate
of the whole river. The same thing is encountered where the currents through R1, R2, and R3
join to flow back to the positive terminal of the battery (+) toward point 1: the flow of electrons
from point 2 to point 1 must equal the sum of the (branch) currents through R1, R2, and R3.
5.3.
SIMPLE PARALLEL CIRCUITS
141
This is the second principle of parallel circuits: the total circuit current is equal to the sum
of the individual branch currents. Using this principle, we can fill in the IT spot on our table
with the sum of IR1, IR2, and IR3:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
10k
2k
1k
0.9m
4.5m
9m
Rule of parallel
circuits
14.4m
Finally, applying Ohm's Law to the rightmost ("Total") column, we can calculate the total
circuit resistance:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
10k
2k
1k
0.9m
4.5m
9m
14.4m
625
Ohm's
Law
Etotal
Itotal
9 V
14.4 mA
Please note something very important here.
The total circuit resistance is only 625 Ω:
less than any one of the individual resistors. In the series circuit, where the total resistance
was the sum of the individual resistances, the total was bound to be greater than any one of the
resistors individually. Here in the parallel circuit, however, the opposite is true: we say that the
individual resistances diminish rather than add to make the total. This principle completes
our triad of "rules" for parallel circuits, just as series circuits were found to have three rules
for voltage, current, and resistance. Mathematically, the relationship between total resistance
and individual resistances in a parallel circuit looks like this:
R1
R2
R3
1
1
1
1
The same basic form of equation works for any number of resistors connected together in
parallel, just add as many 1/R terms on the denominator of the fraction as needed to accom-
modate all parallel resistors in the circuit.
Just as with the series circuit, we can use computer analysis to double-check our calcula-
tions. First, of course, we have to describe our example circuit to the computer in terms it can
understand. I'll start by re-drawing the circuit:
142
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
1
2
3
4
5
6
7
8
R1
R2
R3
10 kΩ
2 kΩ
1 kΩ
9 V
Once again we find that the original numbering scheme used to identify points in the circuit
will have to be altered for the benefit of SPICE. In SPICE, all electrically common points must
share identical node numbers. This is how SPICE knows what's connected to what, and how.
In a simple parallel circuit, all points are electrically common in one of two sets of points. For
our example circuit, the wire connecting the tops of all the components will have one node
number and the wire connecting the bottoms of the components will have the other. Staying
true to the convention of including zero as a node number, I choose the numbers 0 and 1:
1
0
0
0
0
1
1
1
R1
R2
R3
10 kΩ
2 kΩ
1 kΩ
9 V
An example like this makes the rationale of node numbers in SPICE fairly clear to under-
stand. By having all components share common sets of numbers, the computer "knows" they're
all connected in parallel with each other.
In order to display branch currents in SPICE, we need to insert zero-voltage sources in line
(in series) with each resistor, and then reference our current measurements to those sources.
For whatever reason, the creators of the SPICE program made it so that current could only be
calculated through a voltage source. This is a somewhat annoying demand of the SPICE sim-
ulation program. With each of these "dummy" voltage sources added, some new node numbers
must be created to connect them to their respective branch resistors:
5.3.
SIMPLE PARALLEL CIRCUITS
143
1
0
0
0
0
1
1
1
2
3
4
vr1
vr2
vr3
NOTE: vr1, vr2, and vr3 are all
"dummy" voltage sources with
values of 0 volts each!!
R1
R2
R3
10 kΩ
2 kΩ
1 kΩ
9 V
The dummy voltage sources are all set at 0 volts so as to have no impact on the operation
of the circuit. The circuit description file, or netlist, looks like this:
Parallel circuit
v1 1 0
r1 2 0 10k
r2 3 0 2k
r3 4 0 1k
vr1 1 2 dc 0
vr2 1 3 dc 0
vr3 1 4 dc 0
.dc v1 9 9 1
.print dc v(2,0) v(3,0) v(4,0)
.print dc i(vr1) i(vr2) i(vr3)
.end
Running the computer analysis, we get these results (I've annotated the printout with de-
scriptive labels):
v1
v(2)
v(3)
v(4)
9.000E+00
9.000E+00
9.000E+00
9.000E+00
battery
R1 voltage
R2 voltage
R3 voltage
voltage
v1
i(vr1)
i(vr2)
i(vr3)
9.000E+00
9.000E-04
4.500E-03
9.000E-03
battery
R1 current
R2 current
R3 current
voltage
These values do indeed match those calculated through Ohm's Law earlier: 0.9 mA for IR1,
144
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
4.5 mA for IR2, and 9 mA for IR3. Being connected in parallel, of course, all resistors have the
same voltage dropped across them (9 volts, same as the battery).
In summary, a parallel circuit is defined as one where all components are connected between
the same set of electrically common points. Another way of saying this is that all components
are connected across each other's terminals. From this definition, three rules of parallel circuits
follow: all components share the same voltage; resistances diminish to equal a smaller, total
resistance; and branch currents add to equal a larger, total current. Just as in the case of series
circuits, all of these rules find root in the definition of a parallel circuit. If you understand that
definition fully, then the rules are nothing more than footnotes to the definition.
• REVIEW:
• Components in a parallel circuit share the same voltage: ET otal = E1 = E2 = . . . En
• Total resistance in a parallel circuit is less than any of the individual resistances: RT otal
• Total current in a parallel circuit is equal to the sum of the individual branch currents:
5.4
Conductance
When students first see the parallel resistance equation, the natural question to ask is, "Where
did that thing come from?" It is truly an odd piece of arithmetic, and its origin deserves a good
explanation.
Resistance, by definition, is the measure of friction a component presents to the flow of
electrons through it. Resistance is symbolized by the capital letter "R" and is measured in the
unit of "ohm." However, we can also think of this electrical property in terms of its inverse:
how easy it is for electrons to flow through a component, rather than how difficult. If resistance
is the word we use to symbolize the measure of how difficult it is for electrons to flow, then a
good word to express how easy it is for electrons to flow would be conductance.
Mathematically, conductance is the reciprocal, or inverse, of resistance:
Conductance =
Resistance
1
The greater the resistance, the less the conductance, and vice versa. This should make
intuitive sense, resistance and conductance being opposite ways to denote the same essential
electrical property. If two components' resistances are compared and it is found that compo-
nent "A" has one-half the resistance of component "B," then we could alternatively express this
relationship by saying that component "A" is twice as conductive as component "B." If compo-
nent "A" has but one-third the resistance of component "B," then we could say it is three times
more conductive than component "B," and so on.
Carrying this idea further, a symbol and unit were created to represent conductance. The
symbol is the capital letter "G" and the unit is the mho, which is "ohm" spelled backwards (and
you didn't think electronics engineers had any sense of humor!). Despite its appropriateness,
the unit of the mho was replaced in later years by the unit of siemens (abbreviated by the
5.4.
CONDUCTANCE
145
capital letter "S"). This decision to change unit names is reminiscent of the change from the
temperature unit of degrees Centigrade to degrees Celsius, or the change from the unit of
frequency c.p.s. (cycles per second) to Hertz. If you're looking for a pattern here, Siemens,
Celsius, and Hertz are all surnames of famous scientists, the names of which, sadly, tell us less
about the nature of the units than the units' original designations.
As a footnote, the unit of siemens is never expressed without the last letter "s." In other
words, there is no such thing as a unit of "siemen" as there is in the case of the "ohm" or
the "mho." The reason for this is the proper spelling of the respective scientists' surnames.
The unit for electrical resistance was named after someone named "Ohm," whereas the unit
for electrical conductance was named after someone named "Siemens," therefore it would be
improper to "singularize" the latter unit as its final "s" does not denote plurality.
Back to our parallel circuit example, we should be able to see that multiple paths (branches)
for current reduces total resistance for the whole circuit, as electrons are able to flow easier
through the whole network of multiple branches than through any one of those branch re-
sistances alone. In terms of resistance, additional branches result in a lesser total (current
meets with less opposition). In terms of conductance, however, additional branches results in
a greater total (electrons flow with greater conductance):
Total parallel resistance is less than any one of the individual branch resistances because
parallel resistors resist less together than they would separately:
Rtotal
Rtotal is less than R1, R2, R3, or R4 individually
R1
R2
R3
R4
Total parallel conductance is greater than any of the individual branch conductances be-
cause parallel resistors conduct better together than they would separately:
Gtotal
G1
G2
G3
G4
Gtotal is greater than G1, G2, G3, or G4 individually
To be more precise, the total conductance in a parallel circuit is equal to the sum of the
individual conductances:
If we know that conductance is nothing more than the mathematical reciprocal (1/x) of
resistance, we can translate each term of the above formula into resistance by substituting the
reciprocal of each respective conductance:
146
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
R1
R2
R3
1
1
1
1
Rtotal
1
R4
Solving the above equation for total resistance (instead of the reciprocal of total resistance),
we can invert (reciprocate) both sides of the equation:
R1
R2
R3
1
1
1
1
1
R4
So, we arrive at our cryptic resistance formula at last! Conductance (G) is seldom used as
a practical measurement, and so the above formula is a common one to see in the analysis of
parallel circuits.
• REVIEW:
• Conductance is the opposite of resistance: the measure of how easy it is for electrons to
flow through something.
• Conductance is symbolized with the letter "G" and is measured in units of mhos or
Siemens.
• Mathematically, conductance equals the reciprocal of resistance: G = 1/R
5.5
Power calculations
When calculating the power dissipation of resistive components, use any one of the three power
equations to derive the answer from values of voltage, current, and/or resistance pertaining to
each component:
E
R
E2
I2R
Power equations
This is easily managed by adding another row to our familiar table of voltages, currents,
and resistances:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
P
Watts
Power for any particular table column can be found by the appropriate Ohm's Law equation
(appropriate based on what figures are present for E, I, and R in that column).
5.6.
CORRECT USE OF OHM'S LAW
147
An interesting rule for total power versus individual power is that it is additive for any
configuration of circuit: series, parallel, series/parallel, or otherwise. Power is a measure of
rate of work, and since power dissipated must equal the total power applied by the source(s)
(as per the Law of Conservation of Energy in physics), circuit configuration has no effect on the
mathematics.
• REVIEW:
• Power is additive in any configuration of resistive circuit: PT otal = P1 + P2 + . . . Pn
5.6
Correct use of Ohm's Law
One of the most common mistakes made by beginning electronics students in their application
of Ohm's Laws is mixing the contexts of voltage, current, and resistance. In other words, a
student might mistakenly use a value for I through one resistor and the value for E across a
set of interconnected resistors, thinking that they'll arrive at the resistance of that one resistor.
Not so! Remember this important rule: The variables used in Ohm's Law equations must be
common to the same two points in the circuit under consideration. I cannot overemphasize
this rule. This is especially important in series-parallel combination circuits where nearby
components may have different values for both voltage drop and current.
When using Ohm's Law to calculate a variable pertaining to a single component, be sure
the voltage you're referencing is solely across that single component and the current you're
referencing is solely through that single component and the resistance you're referencing is
solely for that single component. Likewise, when calculating a variable pertaining to a set of
components in a circuit, be sure that the voltage, current, and resistance values are specific to
that complete set of components only! A good way to remember this is to pay close attention
to the two points terminating the component or set of components being analyzed, making
sure that the voltage in question is across those two points, that the current in question is
the electron flow from one of those points all the way to the other point, that the resistance in
question is the equivalent of a single resistor between those two points, and that the power in
question is the total power dissipated by all components between those two points.
The "table" method presented for both series and parallel circuits in this chapter is a good
way to keep the context of Ohm's Law correct for any kind of circuit configuration. In a table
like the one shown below, you are only allowed to apply an Ohm's Law equation for the values
of a single vertical column at a time:
148
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
P
Watts
Ohm's
Law
Ohm's
Law
Ohm's
Law
Ohm's
Law
Deriving values horizontally across columns is allowable as per the principles of series and
parallel circuits:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
P
Watts
For series circuits:
Add
Equal
Add
Add
5.7.
COMPONENT FAILURE ANALYSIS
149
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
P
Watts
Equal
Add
Add
For parallel circuits:
Diminish
R1
R2
R3
1
1
1
1
Not only does the "table" method simplify the management of all relevant quantities, it also
facilitates cross-checking of answers by making it easy to solve for the original unknown vari-
ables through other methods, or by working backwards to solve for the initially given values
from your solutions. For example, if you have just solved for all unknown voltages, currents,
and resistances in a circuit, you can check your work by adding a row at the bottom for power
calculations on each resistor, seeing whether or not all the individual power values add up to
the total power. If not, then you must have made a mistake somewhere! While this technique
of "cross-checking" your work is nothing new, using the table to arrange all the data for the
cross-check(s) results in a minimum of confusion.
• REVIEW:
• Apply Ohm's Law to vertical columns in the table.
• Apply rules of series/parallel to horizontal rows in the table.
• Check your calculations by working "backwards" to try to arrive at originally given values
(from your first calculated answers), or by solving for a quantity using more than one
method (from different given values).
5.7
Component failure analysis
The job of a technician frequently entails "troubleshooting" (locating and correcting a prob-
lem) in malfunctioning circuits. Good troubleshooting is a demanding and rewarding effort,
requiring a thorough understanding of the basic concepts, the ability to formulate hypotheses
(proposed explanations of an effect), the ability to judge the value of different hypotheses based
150
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
on their probability (how likely one particular cause may be over another), and a sense of cre-
ativity in applying a solution to rectify the problem. While it is possible to distill these skills
into a scientific methodology, most practiced troubleshooters would agree that troubleshooting
involves a touch of art, and that it can take years of experience to fully develop this art.
An essential skill to have is a ready and intuitive understanding of how component faults
affect circuits in different configurations. We will explore some of the effects of component
faults in both series and parallel circuits here, then to a greater degree at the end of the
"Series-Parallel Combination Circuits" chapter.
Let's start with a simple series circuit:
R1
R2
R3
100 Ω
300 Ω
50 Ω
9 V
With all components in this circuit functioning at their proper values, we can mathemati-
cally determine all currents and voltage drops:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
100
300
50
450
9
20m
20m
20m
20m
2
6
1
Now let us suppose that R2 fails shorted. Shorted means that the resistor now acts like a
straight piece of wire, with little or no resistance. The circuit will behave as though a "jumper"
wire were connected across R2 (in case you were wondering, "jumper wire" is a common term
for a temporary wire connection in a circuit). What causes the shorted condition of R2 is no
matter to us in this example; we only care about its effect upon the circuit:
R1
R2
R3
100 Ω
300 Ω
50 Ω
9 V
jumper wire
With R2 shorted, either by a jumper wire or by an internal resistor failure, the total circuit
resistance will decrease. Since the voltage output by the battery is a constant (at least in our
ideal simulation here), a decrease in total circuit resistance means that total circuit current
5.7.
COMPONENT FAILURE ANALYSIS
151
must increase:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
100
50
9
Shorted
resistor
0
60m
60m
60m
60m
150
0
6
3
As the circuit current increases from 20 milliamps to 60 milliamps, the voltage drops across
R1 and R3 (which haven't changed resistances) increase as well, so that the two resistors are
dropping the whole 9 volts. R2, being bypassed by the very low resistance of the jumper wire,
is effectively eliminated from the circuit, the resistance from one lead to the other having been
reduced to zero. Thus, the voltage drop across R2, even with the increased total current, is zero
volts.
On the other hand, if R2 were to fail "open" - resistance increasing to nearly infinite levels
- it would also create wide-reaching effects in the rest of the circuit:
R1
R2
R3
100 Ω
300 Ω
50 Ω
9 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
100
50
9
resistor
9
0
0
0
0
0
0
Open
With R2 at infinite resistance and total resistance being the sum of all individual resistances
in a series circuit, the total current decreases to zero. With zero circuit current, there is no
electron flow to produce voltage drops across R1 or R3. R2, on the other hand, will manifest the
full supply voltage across its terminals.
We can apply the same before/after analysis technique to parallel circuits as well. First, we
determine what a "healthy" parallel circuit should behave like.
152
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
R1
R2
R3
90 Ω
45 Ω
180 Ω
9 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
90
45
180
25.714
350m
100m
200m
50m
Supposing that R2 opens in this parallel circuit, here's what the effects will be:
R1
R2
R3
90 Ω
45 Ω
180 Ω
9 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
90
180
100m
50m
0
150m
60
Open
resistor
Notice that in this parallel circuit, an open branch only affects the current through that
branch and the circuit's total current. Total voltage - being shared equally across all compo-
nents in a parallel circuit, will be the same for all resistors. Due to the fact that the voltage
source's tendency is to hold voltage constant, its voltage will not change, and being in parallel
with all the resistors, it will hold all the resistors' voltages the same as they were before: 9
volts. Being that voltage is the only common parameter in a parallel circuit, and the other re-
sistors haven't changed resistance value, their respective branch currents remain unchanged.
This is what happens in a household lamp circuit: all lamps get their operating voltage
from power wiring arranged in a parallel fashion. Turning one lamp on and off (one branch
in that parallel circuit closing and opening) doesn't affect the operation of other lamps in the
room, only the current in that one lamp (branch circuit) and the total current powering all the
lamps in the room:
5.7.
COMPONENT FAILURE ANALYSIS
153
120
V
In an ideal case (with perfect voltage sources and zero-resistance connecting wire), shorted
resistors in a simple parallel circuit will also have no effect on what's happening in other
branches of the circuit. In real life, the effect is not quite the same, and we'll see why in the
following example:
9 V
R1
R2
R3
90 Ω
45 Ω
180 Ω
R2 "shorted" with a jumper wire
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
9
9
9
9
90
180
100m
50m
0
resistor
Shorted
0
A shorted resistor (resistance of 0 Ω) would theoretically draw infinite current from any
finite source of voltage (I=E/0). In this case, the zero resistance of R2 decreases the circuit
total resistance to zero Ωas well, increasing total current to a value of infinity. As long as the
voltage source holds steady at 9 volts, however, the other branch currents (IR1 and IR3) will
remain unchanged.
The critical assumption in this "perfect" scheme, however, is that the voltage supply will
hold steady at its rated voltage while supplying an infinite amount of current to a short-circuit
load. This is simply not realistic. Even if the short has a small amount of resistance (as opposed
to absolutely zero resistance), no real voltage source could arbitrarily supply a huge overload
current and maintain steady voltage at the same time. This is primarily due to the internal
resistance intrinsic to all electrical power sources, stemming from the inescapable physical
properties of the materials they're constructed of:
154
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
9 V
Rinternal
Battery
These internal resistances, small as they may be, turn our simple parallel circuit into a
series-parallel combination circuit. Usually, the internal resistances of voltage sources are
low enough that they can be safely ignored, but when high currents resulting from shorted
components are encountered, their effects become very noticeable.
In this case, a shorted
R2 would result in almost all the voltage being dropped across the internal resistance of the
battery, with almost no voltage left over for resistors R1, R2, and R3:
9 V
R1
R2
R3
90 Ω
45 Ω
180 Ω
R2 "shorted" with a jumper wire
Rinternal
Battery
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
90
180
0
resistor
Shorted
0
low
low
low
low
low
low
high
high
Supply voltage
decrease due to
voltage drop across
internal resistance
Suffice it to say, intentional direct short-circuits across the terminals of any voltage source
is a bad idea. Even if the resulting high current (heat, flashes, sparks) causes no harm to
people nearby, the voltage source will likely sustain damage, unless it has been specifically
designed to handle short-circuits, which most voltage sources are not.
Eventually in this book I will lead you through the analysis of circuits without the use of
any numbers, that is, analyzing the effects of component failure in a circuit without knowing
exactly how many volts the battery produces, how many ohms of resistance is in each resistor,
etc. This section serves as an introductory step to that kind of analysis.
Whereas the normal application of Ohm's Law and the rules of series and parallel circuits is
performed with numerical quantities ("quantitative"), this new kind of analysis without precise
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
155
numerical figures is something I like to call qualitative analysis. In other words, we will be
analyzing the qualities of the effects in a circuit rather than the precise quantities. The result,
for you, will be a much deeper intuitive understanding of electric circuit operation.
• REVIEW:
• To determine what would happen in a circuit if a component fails, re-draw that circuit
with the equivalent resistance of the failed component in place and re-calculate all values.
• The ability to intuitively determine what will happen to a circuit with any given compo-
nent fault is a crucial skill for any electronics troubleshooter to develop. The best way to
learn is to experiment with circuit calculations and real-life circuits, paying close atten-
tion to what changes with a fault, what remains the same, and why!
• A shorted component is one whose resistance has dramatically decreased.
• An open component is one whose resistance has dramatically increased. For the record,
resistors tend to fail open more often than fail shorted, and they almost never fail unless
physically or electrically overstressed (physically abused or overheated).
5.8
Building simple resistor circuits
In the course of learning about electricity, you will want to construct your own circuits using
resistors and batteries. Some options are available in this matter of circuit assembly, some
easier than others. In this section, I will explore a couple of fabrication techniques that will
not only help you build the circuits shown in this chapter, but also more advanced circuits.
If all we wish to construct is a simple single-battery, single-resistor circuit, we may easily
use alligator clip jumper wires like this:
156
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
Battery
Resistor
Schematic
diagram
Real circuit using jumper wires
Jumper wires with "alligator" style spring clips at each end provide a safe and convenient
method of electrically joining components together.
If we wanted to build a simple series circuit with one battery and three resistors, the same
"point-to-point" construction technique using jumper wires could be applied:
Battery
Schematic
diagram
Real circuit using jumper wires
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
157
This technique, however, proves impractical for circuits much more complex than this, due
to the awkwardness of the jumper wires and the physical fragility of their connections. A more
common method of temporary construction for the hobbyist is the solderless breadboard, a
device made of plastic with hundreds of spring-loaded connection sockets joining the inserted
ends of components and/or 22-gauge solid wire pieces. A photograph of a real breadboard is
shown here, followed by an illustration showing a simple series circuit constructed on one:
Battery
Schematic
diagram
Real circuit using a solderless breadboard
Underneath each hole in the breadboard face is a metal spring clip, designed to grasp any
inserted wire or component lead. These metal spring clips are joined underneath the bread-
board face, making connections between inserted leads. The connection pattern joins every
five holes along a vertical column (as shown with the long axis of the breadboard situated
horizontally):
158
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
Lines show common connections
underneath board between holes
Thus, when a wire or component lead is inserted into a hole on the breadboard, there are
four more holes in that column providing potential connection points to other wires and/or com-
ponent leads. The result is an extremely flexible platform for constructing temporary circuits.
For example, the three-resistor circuit just shown could also be built on a breadboard like this:
Battery
Schematic
diagram
Real circuit using a solderless breadboard
A parallel circuit is also easy to construct on a solderless breadboard:
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
159
Battery
Schematic
diagram
Real circuit using a solderless breadboard
Breadboards have their limitations, though. First and foremost, they are intended for tem-
porary construction only. If you pick up a breadboard, turn it upside-down, and shake it, any
components plugged into it are sure to loosen, and may fall out of their respective holes. Also,
breadboards are limited to fairly low-current (less than 1 amp) circuits. Those spring clips
have a small contact area, and thus cannot support high currents without excessive heating.
For greater permanence, one might wish to choose soldering or wire-wrapping. These tech-
niques involve fastening the components and wires to some structure providing a secure me-
chanical location (such as a phenolic or fiberglass board with holes drilled in it, much like
a breadboard without the intrinsic spring-clip connections), and then attaching wires to the
secured component leads. Soldering is a form of low-temperature welding, using a tin/lead
or tin/silver alloy that melts to and electrically bonds copper objects. Wire ends soldered to
component leads or to small, copper ring "pads" bonded on the surface of the circuit board
serve to connect the components together. In wire wrapping, a small-gauge wire is tightly
wrapped around component leads rather than soldered to leads or copper pads, the tension of
the wrapped wire providing a sound mechanical and electrical junction to connect components
together.
An example of a printed circuit board, or PCB, intended for hobbyist use is shown in this
photograph:
160
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
This board appears copper-side-up: the side where all the soldering is done. Each hole is
ringed with a small layer of copper metal for bonding to the solder. All holes are independent
of each other on this particular board, unlike the holes on a solderless breadboard which are
connected together in groups of five. Printed circuit boards with the same 5-hole connection
pattern as breadboards can be purchased and used for hobby circuit construction, though.
Production printed circuit boards have traces of copper laid down on the phenolic or fiber-
glass substrate material to form pre-engineered connection pathways which function as wires
in a circuit. An example of such a board is shown here, this unit actually a "power supply"
circuit designed to take 120 volt alternating current (AC) power from a household wall socket
and transform it into low-voltage direct current (DC). A resistor appears on this board, the
fifth component counting up from the bottom, located in the middle-right area of the board.
A view of this board's underside reveals the copper "traces" connecting components together,
as well as the silver-colored deposits of solder bonding the component leads to those traces:
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
161
A soldered or wire-wrapped circuit is considered permanent: that is, it is unlikely to fall
apart accidently. However, these construction techniques are sometimes considered too per-
manent. If anyone wishes to replace a component or change the circuit in any substantial
way, they must invest a fair amount of time undoing the connections. Also, both soldering and
wire-wrapping require specialized tools which may not be immediately available.
An alternative construction technique used throughout the industrial world is that of the
terminal strip. Terminal strips, alternatively called barrier strips or terminal blocks, are com-
prised of a length of nonconducting material with several small bars of metal embedded within.
Each metal bar has at least one machine screw or other fastener under which a wire or compo-
nent lead may be secured. Multiple wires fastened by one screw are made electrically common
to each other, as are wires fastened to multiple screws on the same bar. The following photo-
graph shows one style of terminal strip, with a few wires attached.
Another, smaller terminal strip is shown in this next photograph. This type, sometimes
referred to as a "European" style, has recessed screws to help prevent accidental shorting
between terminals by a screwdriver or other metal object:
162
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
In the following illustration, a single-battery, three-resistor circuit is shown constructed on
a terminal strip:
Series circuit constructed on a
terminal strip
If the terminal strip uses machine screws to hold the component and wire ends, nothing
but a screwdriver is needed to secure new connections or break old connections. Some termi-
nal strips use spring-loaded clips - similar to a breadboard's except for increased ruggedness
- engaged and disengaged using a screwdriver as a push tool (no twisting involved). The elec-
trical connections established by a terminal strip are quite robust, and are considered suitable
for both permanent and temporary construction.
One of the essential skills for anyone interested in electricity and electronics is to be able
to "translate" a schematic diagram to a real circuit layout where the components may not be
oriented the same way. Schematic diagrams are usually drawn for maximum readability (ex-
cepting those few noteworthy examples sketched to create maximum confusion!), but practical
circuit construction often demands a different component orientation. Building simple circuits
on terminal strips is one way to develop the spatial-reasoning skill of "stretching" wires to
make the same connection paths. Consider the case of a single-battery, three-resistor parallel
circuit constructed on a terminal strip:
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
163
Schematic diagram
Real circuit using a terminal strip
Progressing from a nice, neat, schematic diagram to the real circuit - especially when the
resistors to be connected are physically arranged in a linear fashion on the terminal strip
- is not obvious to many, so I'll outline the process step-by-step. First, start with the clean
schematic diagram and all components secured to the terminal strip, with no connecting wires:
164
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
Schematic diagram
Real circuit using a terminal strip
Next, trace the wire connection from one side of the battery to the first component in the
schematic, securing a connecting wire between the same two points on the real circuit. I find
it helpful to over-draw the schematic's wire with another line to indicate what connections I've
made in real life:
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
165
Schematic diagram
Real circuit using a terminal strip
Continue this process, wire by wire, until all connections in the schematic diagram have
been accounted for. It might be helpful to regard common wires in a SPICE-like fashion: make
all connections to a common wire in the circuit as one step, making sure each and every com-
ponent with a connection to that wire actually has a connection to that wire before proceeding
to the next. For the next step, I'll show how the top sides of the remaining two resistors are
connected together, being common with the wire secured in the previous step:
166
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
Schematic diagram
Real circuit using a terminal strip
With the top sides of all resistors (as shown in the schematic) connected together, and to
the battery's positive (+) terminal, all we have to do now is connect the bottom sides together
and to the other side of the battery:
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
167
Schematic diagram
Real circuit using a terminal strip
Typically in industry, all wires are labeled with number tags, and electrically common wires
bear the same tag number, just as they do in a SPICE simulation. In this case, we could label
the wires 1 and 2:
168
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
1
1
1
1
1
1
1
1
1
1
1
1
1
2
2
2
2
2
2
2
2
2
2
2
2
1
1
1
2
2
2
Common wire numbers representing
electrically common points
1
2
Another industrial convention is to modify the schematic diagram slightly so as to indicate
actual wire connection points on the terminal strip. This demands a labeling system for the
strip itself: a "TB" number (terminal block number) for the strip, followed by another number
representing each metal bar on the strip.
5.8.
BUILDING SIMPLE RESISTOR CIRCUITS
169
1
1
1
1
1
1
1
1
1
1
1
1
1
2
2
2
2
2
2
2
2
2
2
2
2
1
1
1
2
2
2
1
2
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
TB1
Terminal strip bars labeled and
connection points referenced in diagram
This way, the schematic may be used as a "map" to locate points in a real circuit, regardless
of how tangled and complex the connecting wiring may appear to the eyes. This may seem ex-
cessive for the simple, three-resistor circuit shown here, but such detail is absolutely necessary
for construction and maintenance of large circuits, especially when those circuits may span a
great physical distance, using more than one terminal strip located in more than one panel or
box.
• REVIEW:
• A solderless breadboard is a device used to quickly assemble temporary circuits by plug-
ging wires and components into electrically common spring-clips arranged underneath
rows of holes in a plastic board.
• Soldering is a low-temperature welding process utilizing a lead/tin or tin/silver alloy to
bond wires and component leads together, usually with the components secured to a fiber-
glass board.
• Wire-wrapping is an alternative to soldering, involving small-gauge wire tightly wrapped
around component leads rather than a welded joint to connect components together.
• A terminal strip, also known as a barrier strip or terminal block is another device used
to mount components and wires to build circuits. Screw terminals or heavy spring clips
attached to metal bars provide connection points for the wire ends and component leads,
these metal bars mounted separately to a piece of nonconducting material such as plastic,
bakelite, or ceramic.
170
CHAPTER 5. SERIES AND PARALLEL CIRCUITS
5.9
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Ron LaPlante (October 1998): helped create "table" method of series and parallel circuit
analysis.
Chapter 6
DIVIDER CIRCUITS AND
KIRCHHOFF'S LAWS
Contents
6.1
Voltage divider circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 171
6.2
Kirchhoff's Voltage Law (KVL)
. . . . . . . . . . . . . . . . . . . . . . . . . . 179
6.3
Current divider circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 190
6.4
Kirchhoff's Current Law (KCL) . . . . . . . . . . . . . . . . . . . . . . . . . . 193
6.5
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 196
6.1
Voltage divider circuits
Let's analyze a simple series circuit, determining the voltage drops across individual resistors:
R1
R2
R3
5 kΩ
7.5 kΩ
10 kΩ
45 V
171
172
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
5k
10k
7.5k
45
From the given values of individual resistances, we can determine a total circuit resistance,
knowing that resistances add in series:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
5k
10k
7.5k
22.5k
45
From here, we can use Ohm's Law (I=E/R) to determine the total current, which we know
will be the same as each resistor current, currents being equal in all parts of a series circuit:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
5k
10k
7.5k
45
22.5k
2m
2m
2m
2m
Now, knowing that the circuit current is 2 mA, we can use Ohm's Law (E=IR) to calculate
voltage across each resistor:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
5k
10k
7.5k
45
22.5k
2m
2m
2m
2m
10
20
15
It should be apparent that the voltage drop across each resistor is proportional to its resis-
tance, given that the current is the same through all resistors. Notice how the voltage across
R2 is double that of the voltage across R1, just as the resistance of R2 is double that of R1.
If we were to change the total voltage, we would find this proportionality of voltage drops
remains constant:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
5k
10k
7.5k
22.5k
8m
8m
8m
8m
40
80
60
180
The voltage across R2 is still exactly twice that of R1's drop, despite the fact that the source
voltage has changed. The proportionality of voltage drops (ratio of one to another) is strictly a
6.1.
VOLTAGE DIVIDER CIRCUITS
173
function of resistance values.
With a little more observation, it becomes apparent that the voltage drop across each re-
sistor is also a fixed proportion of the supply voltage. The voltage across R1, for example, was
10 volts when the battery supply was 45 volts. When the battery voltage was increased to 180
volts (4 times as much), the voltage drop across R1 also increased by a factor of 4 (from 10 to
40 volts). The ratio between R1's voltage drop and total voltage, however, did not change:
ER1
Etotal
10 V
45 V
40 V
180 V
Likewise, none of the other voltage drop ratios changed with the increased supply voltage
either:
Etotal
ER2
45 V
180 V
80 V
0.44444
20 V
Etotal
45 V
180 V
ER3
15 V
60 V
0.33333
For this reason a series circuit is often called a voltage divider for its ability to proportion
- or divide - the total voltage into fractional portions of constant ratio. With a little bit of
algebra, we can derive a formula for determining series resistor voltage drop given nothing
more than total voltage, individual resistance, and total resistance:
Voltage drop across any resistor
Current in a series circuit
Etotal
Rtotal
Substituting
Etotal
Rtotal
for In in the first equation
Voltage drop across any series resistor
Etotal
Rtotal
Rn
. . . or . . .
Rtotal
Rn
Etotal
The ratio of individual resistance to total resistance is the same as the ratio of individual
voltage drop to total supply voltage in a voltage divider circuit. This is known as the voltage
divider formula, and it is a short-cut method for determining voltage drop in a series circuit
174
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
without going through the current calculation(s) of Ohm's Law.
Using this formula, we can re-analyze the example circuit's voltage drops in fewer steps:
R1
R2
R3
5 kΩ
7.5 kΩ
10 kΩ
45 V
5 kΩ
22.5 kΩ
45 V
22.5 kΩ
10 kΩ
20 V
22.5 kΩ
7.5 kΩ
15 V
Voltage dividers find wide application in electric meter circuits, where specific combinations
of series resistors are used to "divide" a voltage into precise proportions as part of a voltage
measurement device.
Input
voltage
Divided
voltage
R1
R2
One device frequently used as a voltage-dividing component is the potentiometer, which is
a resistor with a movable element positioned by a manual knob or lever. The movable element,
typically called a wiper, makes contact with a resistive strip of material (commonly called the
slidewire if made of resistive metal wire) at any point selected by the manual control:
6.1.
VOLTAGE DIVIDER CIRCUITS
175
1
2
wiper contact
Potentiometer
The wiper contact is the left-facing arrow symbol drawn in the middle of the vertical resistor
element. As it is moved up, it contacts the resistive strip closer to terminal 1 and further away
from terminal 2, lowering resistance to terminal 1 and raising resistance to terminal 2. As it is
moved down, the opposite effect results. The resistance as measured between terminals 1 and
2 is constant for any wiper position.
1
2
less resistance
more resistance
1
2
less resistance
more resistance
Shown here are internal illustrations of two potentiometer types, rotary and linear:
Resistive strip
Wiper
Terminals
Rotary potentiometer
construction
176
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
Resistive strip
Wiper
Terminals
Linear potentiometer construction
Some linear potentiometers are actuated by straight-line motion of a lever or slide button.
Others, like the one depicted in the previous illustration, are actuated by a turn-screw for fine
adjustment ability. The latter units are sometimes referred to as trimpots, because they work
well for applications requiring a variable resistance to be "trimmed" to some precise value.
It should be noted that not all linear potentiometers have the same terminal assignments as
shown in this illustration. With some, the wiper terminal is in the middle, between the two
end terminals.
The following photograph shows a real, rotary potentiometer with exposed wiper and slidewire
for easy viewing. The shaft which moves the wiper has been turned almost fully clockwise so
that the wiper is nearly touching the left terminal end of the slidewire:
Here is the same potentiometer with the wiper shaft moved almost to the full-counterclockwise
position, so that the wiper is near the other extreme end of travel:
6.1.
VOLTAGE DIVIDER CIRCUITS
177
If a constant voltage is applied between the outer terminals (across the length of the
slidewire), the wiper position will tap off a fraction of the applied voltage, measurable between
the wiper contact and either of the other two terminals. The fractional value depends entirely
on the physical position of the wiper:
less voltage
more voltage
Using a potentiometer as a variable voltage divider
Just like the fixed voltage divider, the potentiometer's voltage division ratio is strictly a
function of resistance and not of the magnitude of applied voltage. In other words, if the poten-
tiometer knob or lever is moved to the 50 percent (exact center) position, the voltage dropped
between wiper and either outside terminal would be exactly 1/2 of the applied voltage, no mat-
ter what that voltage happens to be, or what the end-to-end resistance of the potentiometer
is. In other words, a potentiometer functions as a variable voltage divider where the voltage
division ratio is set by wiper position.
This application of the potentiometer is a very useful means of obtaining a variable voltage
from a fixed-voltage source such as a battery. If a circuit you're building requires a certain
amount of voltage that is less than the value of an available battery's voltage, you may connect
the outer terminals of a potentiometer across that battery and "dial up" whatever voltage you
need between the potentiometer wiper and one of the outer terminals for use in your circuit:
178
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
Circuit requiring
less voltage than
what the battery
provides
V
Adjust potentiometer
to obtain desired
voltage
Battery
When used in this manner, the name potentiometer makes perfect sense: they meter (con-
trol) the potential (voltage) applied across them by creating a variable voltage-divider ratio.
This use of the three-terminal potentiometer as a variable voltage divider is very popular in
circuit design.
Shown here are several small potentiometers of the kind commonly used in consumer elec-
tronic equipment and by hobbyists and students in constructing circuits:
The smaller units on the very left and very right are designed to plug into a solderless
breadboard or be soldered into a printed circuit board. The middle units are designed to be
mounted on a flat panel with wires soldered to each of the three terminals.
Here are three more potentiometers, more specialized than the set just shown:
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
179
The large "Helipot" unit is a laboratory potentiometer designed for quick and easy connec-
tion to a circuit. The unit in the lower-left corner of the photograph is the same type of poten-
tiometer, just without a case or 10-turn counting dial. Both of these potentiometers are preci-
sion units, using multi-turn helical-track resistance strips and wiper mechanisms for making
small adjustments. The unit on the lower-right is a panel-mount potentiometer, designed for
rough service in industrial applications.
• REVIEW:
• Series circuits proportion, or divide, the total supply voltage among individual voltage
drops, the proportions being strictly dependent upon resistances: ERn = ET otal (Rn /
RT otal)
• A potentiometer is a variable-resistance component with three connection points, fre-
quently used as an adjustable voltage divider.
6.2
Kirchhoff's Voltage Law (KVL)
Let's take another look at our example series circuit, this time numbering the points in the
circuit for voltage reference:
1
2
3
4
R1
R2
R3
5 kΩ
10 kΩ
7.5 k Ω
45 V
180
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
If we were to connect a voltmeter between points 2 and 1, red test lead to point 2 and black
test lead to point 1, the meter would register +45 volts. Typically the "+" sign is not shown,
but rather implied, for positive readings in digital meter displays. However, for this lesson the
polarity of the voltage reading is very important and so I will show positive numbers explicitly:
When a voltage is specified with a double subscript (the characters "2-1" in the notation
"E2−1"), it means the voltage at the first point (2) as measured in reference to the second point
(1). A voltage specified as "Ecd" would mean the voltage as indicated by a digital meter with
the red test lead on point "c" and the black test lead on point "d": the voltage at "c" in reference
to "d".
COM
A
V
V
A
A
OFF
c
d
Ecd
The meaning of
Red
Black
If we were to take that same voltmeter and measure the voltage drop across each resistor,
stepping around the circuit in a clockwise direction with the red test lead of our meter on
the point ahead and the black test lead on the point behind, we would obtain the following
readings:
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
181
1
2
3
4
R1
R2
R3
5 kΩ
10 kΩ
7.5 k Ω
45 V
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
We should already be familiar with the general principle for series circuits stating that
individual voltage drops add up to the total applied voltage, but measuring voltage drops in
this manner and paying attention to the polarity (mathematical sign) of the readings reveals
another facet of this principle: that the voltages measured as such all add up to zero:
0 V
voltage from point to point 1
2
voltage from point to point
voltage from point to point
voltage from point to point
2
3
3
4
4
1
This principle is known as Kirchhoff's Voltage Law (discovered in 1847 by Gustav R. Kirch-
hoff, a German physicist), and it can be stated as such:
"The algebraic sum of all voltages in a loop must equal zero"
By algebraic, I mean accounting for signs (polarities) as well as magnitudes. By loop, I
mean any path traced from one point in a circuit around to other points in that circuit, and
finally back to the initial point. In the above example the loop was formed by following points
in this order: 1-2-3-4-1. It doesn't matter which point we start at or which direction we proceed
in tracing the loop; the voltage sum will still equal zero. To demonstrate, we can tally up the
voltages in loop 3-2-1-4-3 of the same circuit:
182
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
0 V
voltage from point to point
voltage from point to point
voltage from point to point
voltage from point to point
3
2
2
1
1
4
4
3
This may make more sense if we re-draw our example series circuit so that all components
are represented in a straight line:
1
2
3
4
2
current
current
R1
R2
R3
45 V
5 kΩ
10 kΩ
7.5 kΩ
It's still the same series circuit, just with the components arranged in a different form.
Notice the polarities of the resistor voltage drops with respect to the battery: the battery's
voltage is negative on the left and positive on the right, whereas all the resistor voltage drops
are oriented the other way: positive on the left and negative on the right. This is because the
resistors are resisting the flow of electrons being pushed by the battery. In other words, the
"push" exerted by the resistors against the flow of electrons must be in a direction opposite the
source of electromotive force.
Here we see what a digital voltmeter would indicate across each component in this circuit,
black lead on the left and red lead on the right, as laid out in horizontal fashion:
1
2
3
4
2
current
R1
R2
R3
45 V
5 kΩ
10 kΩ
7.5 kΩ
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
If we were to take that same voltmeter and read voltage across combinations of components,
starting with only R1 on the left and progressing across the whole string of components, we will
see how the voltages add algebraically (to zero):
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
183
1
2
3
4
2
current
R1
R2
R3
45 V
5 kΩ
10 kΩ
7.5 kΩ
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
V Ω
COM
A
0
0 V
The fact that series voltages add up should be no mystery, but we notice that the polarity of
these voltages makes a lot of difference in how the figures add. While reading voltage across
R1, R1−−R2, and R1−−R2−−R3 (I'm using a "double-dash" symbol "−−" to represent the series
connection between resistors R1, R2, and R3), we see how the voltages measure successively
larger (albeit negative) magnitudes, because the polarities of the individual voltage drops are
in the same orientation (positive left, negative right). The sum of the voltage drops across R1,
R2, and R3 equals 45 volts, which is the same as the battery's output, except that the battery's
polarity is opposite that of the resistor voltage drops (negative left, positive right), so we end
up with 0 volts measured across the whole string of components.
That we should end up with exactly 0 volts across the whole string should be no mystery,
either. Looking at the circuit, we can see that the far left of the string (left side of R1: point
number 2) is directly connected to the far right of the string (right side of battery: point number
2), as necessary to complete the circuit. Since these two points are directly connected, they are
electrically common to each other. And, as such, the voltage between those two electrically
common points must be zero.
Kirchhoff's Voltage Law (sometimes denoted as KVL for short) will work for any circuit
configuration at all, not just simple series. Note how it works for this parallel circuit:
184
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
1
2
3
4
5
6
7
8
R1
R2
R3
6 V
Being a parallel circuit, the voltage across every resistor is the same as the supply voltage:
6 volts. Tallying up voltages around loop 2-3-4-5-6-7-2, we get:
0 V
voltage from point to point
voltage from point to point
voltage from point to point
2
3
3
4
4
0 V
0 V
5
voltage from point to point
voltage from point to point
voltage from point to point
5
6
6
7
7
2
0 V
0 V
Note how I label the final (sum) voltage as E2−2. Since we began our loop-stepping sequence
at point 2 and ended at point 2, the algebraic sum of those voltages will be the same as the
voltage measured between the same point (E2−2), which of course must be zero.
The fact that this circuit is parallel instead of series has nothing to do with the validity of
Kirchhoff's Voltage Law. For that matter, the circuit could be a "black box" - its component
configuration completely hidden from our view, with only a set of exposed terminals for us to
measure voltage between - and KVL would still hold true:
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
185
5 V
8 V
3 V
11 V
8 V
10 V
2 V
Try any order of steps from any terminal in the above diagram, stepping around back to the
original terminal, and you'll find that the algebraic sum of the voltages always equals zero.
Furthermore, the "loop" we trace for KVL doesn't even have to be a real current path in the
closed-circuit sense of the word. All we have to do to comply with KVL is to begin and end at
the same point in the circuit, tallying voltage drops and polarities as we go between the next
and the last point. Consider this absurd example, tracing "loop" 2-3-6-3-2 in the same parallel
resistor circuit:
1
2
3
4
5
6
7
8
R1
R2
R3
6 V
0 V
voltage from point to point
voltage from point to point
2
3
0 V
voltage from point to point
voltage from point to point
6
6
2
0 V
3
3
3
186
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
KVL can be used to determine an unknown voltage in a complex circuit, where all other
voltages around a particular "loop" are known. Take the following complex circuit (actually
two series circuits joined by a single wire at the bottom) as an example:
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
To make the problem simpler, I've omitted resistance values and simply given voltage drops
across each resistor. The two series circuits share a common wire between them (wire 7-8-9-10),
making voltage measurements between the two circuits possible. If we wanted to determine
the voltage between points 4 and 3, we could set up a KVL equation with the voltage between
those points as the unknown:
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
187
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
Measuring voltage from point 4 to point 3 (unknown amount)
V Ω
COM
A
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
V Ω
COM
A
Measuring voltage from point 9 to point 4 (+12 volts)
188
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
V Ω
COM
A
0
Measuring voltage from point 8 to point 9 (0 volts)
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
V Ω
COM
A
Measuring voltage from point 3 to point 8 (+20 volts)
Stepping around the loop 3-4-9-8-3, we write the voltage drop figures as a digital voltmeter
would register them, measuring with the red test lead on the point ahead and black test lead
on the point behind as we progress around the loop. Therefore, the voltage from point 9 to
point 4 is a positive (+) 12 volts because the "red lead" is on point 9 and the "black lead" is on
point 4. The voltage from point 3 to point 8 is a positive (+) 20 volts because the "red lead" is on
point 3 and the "black lead" is on point 8. The voltage from point 8 to point 9 is zero, of course,
because those two points are electrically common.
Our final answer for the voltage from point 4 to point 3 is a negative (-) 32 volts, telling
us that point 3 is actually positive with respect to point 4, precisely what a digital voltmeter
6.2.
KIRCHHOFF'S VOLTAGE LAW (KVL)
189
would indicate with the red lead on point 4 and the black lead on point 3:
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
V Ω
COM
A
In other words, the initial placement of our "meter leads" in this KVL problem was "back-
wards." Had we generated our KVL equation starting with E3−4 instead of E4−3, stepping
around the same loop with the opposite meter lead orientation, the final answer would have
been E3−4 = +32 volts:
1
2
3
4
5
6
7
8
9
10
35 V
15 V
20 V
13 V
12 V
25 V
V Ω
COM
A
It is important to realize that neither approach is "wrong." In both cases, we arrive at the
correct assessment of voltage between the two points, 3 and 4: point 3 is positive with respect
to point 4, and the voltage between them is 32 volts.
• REVIEW:
• Kirchhoff's Voltage Law (KVL): "The algebraic sum of all voltages in a loop must equal
zero"
190
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
6.3
Current divider circuits
Let's analyze a simple parallel circuit, determining the branch currents through individual
resistors:
R1
R2
R3
1 kΩ
3 kΩ
2 kΩ
6 V
Knowing that voltages across all components in a parallel circuit are the same, we can fill
in our voltage/current/resistance table with 6 volts across the top row:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
6
6
6
6
1k
3k
2k
Using Ohm's Law (I=E/R) we can calculate each branch current:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
6
6
6
6
1k
3k
2k
6m
2m
3m
Knowing that branch currents add up in parallel circuits to equal the total current, we can
arrive at total current by summing 6 mA, 2 mA, and 3 mA:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
6
6
6
6
1k
3k
2k
6m
2m
3m
11m
The final step, of course, is to figure total resistance. This can be done with Ohm's Law
(R=E/I) in the "total" column, or with the parallel resistance formula from individual resis-
tances. Either way, we'll get the same answer:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
6
6
6
6
1k
3k
2k
6m
2m
3m
11m
545.45
6.3.
CURRENT DIVIDER CIRCUITS
191
Once again, it should be apparent that the current through each resistor is related to its re-
sistance, given that the voltage across all resistors is the same. Rather than being directly pro-
portional, the relationship here is one of inverse proportion. For example, the current through
R1 is twice as much as the current through R3, which has twice the resistance of R1.
If we were to change the supply voltage of this circuit, we find that (surprise!) these pro-
portional ratios do not change:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
1k
3k
2k
545.45
24
24
24
24
24m
8m
12m
44m
The current through R1 is still exactly twice that of R3, despite the fact that the source volt-
age has changed. The proportionality between different branch currents is strictly a function
of resistance.
Also reminiscent of voltage dividers is the fact that branch currents are fixed proportions of
the total current. Despite the fourfold increase in supply voltage, the ratio between any branch
current and the total current remains unchanged:
IR1
Itotal
Itotal
11 mA
11 mA
44 mA
44 mA
IR2
6 mA
24 mA
2 mA
8 mA
0.54545
0.18182
Itotal
11 mA
44 mA
IR3
3 mA
12 mA
0.27273
For this reason a parallel circuit is often called a current divider for its ability to proportion
- or divide - the total current into fractional parts. With a little bit of algebra, we can derive
a formula for determining parallel resistor current given nothing more than total current,
individual resistance, and total resistance:
192
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
Current through any resistor
En
Rn
Voltage in a parallel circuit
Etotal = En = Itotal Rtotal
Substituting
Itotal Rtotal for En in the first equation . . .
Current through any parallel resistor
Rn
Itotal Rtotal
. . . or . . .
In = Itotal
Rn
Rtotal
The ratio of total resistance to individual resistance is the same ratio as individual (branch)
current to total current. This is known as the current divider formula, and it is a short-cut
method for determining branch currents in a parallel circuit when the total current is known.
Using the original parallel circuit as an example, we can re-calculate the branch currents
using this formula, if we start by knowing the total current and total resistance:
545.45 Ω
11 mA
1 kΩ
11 mA
545.45 Ω
11 mA
545.45 Ω
3 kΩ
2 mA
2 kΩ
3 mA
If you take the time to compare the two divider formulae, you'll see that they are remark-
ably similar. Notice, however, that the ratio in the voltage divider formula is Rn (individual
resistance) divided by RT otal, and how the ratio in the current divider formula is RT otal divided
by Rn:
6.4.
KIRCHHOFF'S CURRENT LAW (KCL)
193
Rtotal
Rn
Etotal
In = Itotal
Rn
Rtotal
Voltage divider
formula
formula
Current divider
It is quite easy to confuse these two equations, getting the resistance ratios backwards. One
way to help remember the proper form is to keep in mind that both ratios in the voltage and
current divider equations must equal less than one. After all these are divider equations, not
multiplier equations! If the fraction is upside-down, it will provide a ratio greater than one,
which is incorrect. Knowing that total resistance in a series (voltage divider) circuit is always
greater than any of the individual resistances, we know that the fraction for that formula must
be Rn over RT otal. Conversely, knowing that total resistance in a parallel (current divider)
circuit is always less then any of the individual resistances, we know that the fraction for that
formula must be RT otal over Rn.
Current divider circuits also find application in electric meter circuits, where a fraction of
a measured current is desired to be routed through a sensitive detection device. Using the
current divider formula, the proper shunt resistor can be sized to proportion just the right
amount of current for the device in any given instance:
sensitive device
fraction of total
current
Rshunt
Itotal
Itotal
• REVIEW:
• Parallel circuits proportion, or "divide," the total circuit current among individual branch
currents, the proportions being strictly dependent upon resistances: In = IT otal (RT otal /
Rn)
6.4
Kirchhoff's Current Law (KCL)
Let's take a closer look at that last parallel example circuit:
194
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
1
2
3
4
5
6
7
8
Itotal
Itotal
6 V
R1
R2
R3
1 kΩ
3 kΩ
2 kΩ
IR1
IR2
IR3
Solving for all values of voltage and current in this circuit:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
6
6
6
6
1k
3k
2k
6m
2m
3m
11m
545.45
At this point, we know the value of each branch current and of the total current in the
circuit. We know that the total current in a parallel circuit must equal the sum of the branch
currents, but there's more going on in this circuit than just that. Taking a look at the currents
at each wire junction point (node) in the circuit, we should be able to see something else:
1
2
3
4
5
6
7
8
R1
R2
R3
1 kΩ
3 kΩ
2 kΩ
Itotal
Itotal
IR3
IR3
IR3
IR2
IR1
6 V
At each node on the negative "rail" (wire 8-7-6-5) we have current splitting off the main flow
to each successive branch resistor. At each node on the positive "rail" (wire 1-2-3-4) we have
current merging together to form the main flow from each successive branch resistor. This fact
should be fairly obvious if you think of the water pipe circuit analogy with every branch node
acting as a "tee" fitting, the water flow splitting or merging with the main piping as it travels
from the output of the water pump toward the return reservoir or sump.
If we were to take a closer look at one particular "tee" node, such as node 3, we see that the
current entering the node is equal in magnitude to the current exiting the node:
6.4.
KIRCHHOFF'S CURRENT LAW (KCL)
195
3
IR3
R2
3 kΩ
IR2
From the right and from the bottom, we have two currents entering the wire connection
labeled as node 3. To the left, we have a single current exiting the node equal in magnitude to
the sum of the two currents entering. To refer to the plumbing analogy: so long as there are
no leaks in the piping, what flow enters the fitting must also exit the fitting. This holds true
for any node ("fitting"), no matter how many flows are entering or exiting. Mathematically, we
can express this general relationship as such:
Iexiting = Ientering
Mr. Kirchhoff decided to express it in a slightly different form (though mathematically
equivalent), calling it Kirchhoff's Current Law (KCL):
Summarized in a phrase, Kirchhoff's Current Law reads as such:
"The algebraic sum of all currents entering and exiting a node must
equal zero"
That is, if we assign a mathematical sign (polarity) to each current, denoting whether they
enter (+) or exit (-) a node, we can add them together to arrive at a total of zero, guaranteed.
Taking our example node (number 3), we can determine the magnitude of the current exit-
ing from the left by setting up a KCL equation with that current as the unknown value:
. . . solving for I . . .
The negative (-) sign on the value of 5 milliamps tells us that the current is exiting the
node, as opposed to the 2 milliamp and 3 milliamp currents, which must both be positive (and
therefore entering the node). Whether negative or positive denotes current entering or exiting
is entirely arbitrary, so long as they are opposite signs for opposite directions and we stay
consistent in our notation, KCL will work.
Together, Kirchhoff's Voltage and Current Laws are a formidable pair of tools useful in an-
alyzing electric circuits. Their usefulness will become all the more apparent in a later chapter
196
CHAPTER 6. DIVIDER CIRCUITS AND KIRCHHOFF'S LAWS
("Network Analysis"), but suffice it to say that these Laws deserve to be memorized by the
electronics student every bit as much as Ohm's Law.
• REVIEW:
• Kirchhoff's Current Law (KCL): "The algebraic sum of all currents entering and exiting a
node must equal zero"
6.5
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Ron LaPlante (October 1998): helped create "table" method of series and parallel circuit
analysis.
Chapter 7
SERIES-PARALLEL
COMBINATION CIRCUITS
Contents
7.1
What is a series-parallel circuit?
. . . . . . . . . . . . . . . . . . . . . . . . . 197
7.2
Analysis technique . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 200
7.3
Re-drawing complex schematics
. . . . . . . . . . . . . . . . . . . . . . . . . 208
7.4
Component failure analysis
. . . . . . . . . . . . . . . . . . . . . . . . . . . . 216
7.5
Building series-parallel resistor circuits . . . . . . . . . . . . . . . . . . . . 221
7.6
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 233
7.1
What is a series-parallel circuit?
With simple series circuits, all components are connected end-to-end to form only one path for
electrons to flow through the circuit:
1
2
3
4
R1
R2
R3
Series
197
198
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
With simple parallel circuits, all components are connected between the same two sets of
electrically common points, creating multiple paths for electrons to flow from one end of the
battery to the other:
1
2
3
4
5
6
7
8
R1
R2
R3
Parallel
With each of these two basic circuit configurations, we have specific sets of rules describing
voltage, current, and resistance relationships.
• Series Circuits:
• Voltage drops add to equal total voltage.
• All components share the same (equal) current.
• Resistances add to equal total resistance.
• Parallel Circuits:
• All components share the same (equal) voltage.
• Branch currents add to equal total current.
• Resistances diminish to equal total resistance.
However, if circuit components are series-connected in some parts and parallel in others,
we won't be able to apply a single set of rules to every part of that circuit. Instead, we will have
to identify which parts of that circuit are series and which parts are parallel, then selectively
apply series and parallel rules as necessary to determine what is happening. Take the following
circuit, for instance:
7.1.
WHAT IS A SERIES-PARALLEL CIRCUIT?
199
R1
R2
R3
R4
100 Ω
250 Ω
200 Ω
350 Ω
24 V
A series-parallel combination circuit
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
This circuit is neither simple series nor simple parallel. Rather, it contains elements of both.
The current exits the bottom of the battery, splits up to travel through R3 and R4, rejoins, then
splits up again to travel through R1 and R2, then rejoins again to return to the top of the
battery. There exists more than one path for current to travel (not series), yet there are more
than two sets of electrically common points in the circuit (not parallel).
Because the circuit is a combination of both series and parallel, we cannot apply the rules
for voltage, current, and resistance "across the table" to begin analysis like we could when the
circuits were one way or the other. For instance, if the above circuit were simple series, we
could just add up R1 through R4 to arrive at a total resistance, solve for total current, and
then solve for all voltage drops. Likewise, if the above circuit were simple parallel, we could
just solve for branch currents, add up branch currents to figure the total current, and then
calculate total resistance from total voltage and total current. However, this circuit's solution
will be more complex.
The table will still help us manage the different values for series-parallel combination cir-
cuits, but we'll have to be careful how and where we apply the different rules for series and
parallel. Ohm's Law, of course, still works just the same for determining values within a verti-
cal column in the table.
If we are able to identify which parts of the circuit are series and which parts are parallel,
we can analyze it in stages, approaching each part one at a time, using the appropriate rules
to determine the relationships of voltage, current, and resistance. The rest of this chapter will
be devoted to showing you techniques for doing this.
200
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
• REVIEW:
• The rules of series and parallel circuits must be applied selectively to circuits containing
both types of interconnections.
7.2
Analysis technique
The goal of series-parallel resistor circuit analysis is to be able to determine all voltage drops,
currents, and power dissipations in a circuit. The general strategy to accomplish this goal is
as follows:
• Step 1: Assess which resistors in a circuit are connected together in simple series or
simple parallel.
• Step 2: Re-draw the circuit, replacing each of those series or parallel resistor combi-
nations identified in step 1 with a single, equivalent-value resistor. If using a table to
manage variables, make a new table column for each resistance equivalent.
• Step 3: Repeat steps 1 and 2 until the entire circuit is reduced to one equivalent resistor.
• Step 4: Calculate total current from total voltage and total resistance (I=E/R).
• Step 5: Taking total voltage and total current values, go back to last step in the circuit
reduction process and insert those values where applicable.
• Step 6: From known resistances and total voltage / total current values from step 5, use
Ohm's Law to calculate unknown values (voltage or current) (E=IR or I=E/R).
• Step 7: Repeat steps 5 and 6 until all values for voltage and current are known in the
original circuit configuration. Essentially, you will proceed step-by-step from the sim-
plified version of the circuit back into its original, complex form, plugging in values of
voltage and current where appropriate until all values of voltage and current are known.
• Step 8: Calculate power dissipations from known voltage, current, and/or resistance val-
ues.
This may sound like an intimidating process, but its much easier understood through ex-
ample than through description.
7.2.
ANALYSIS TECHNIQUE
201
R1
R2
R3
R4
100 Ω
250 Ω
200 Ω
350 Ω
24 V
A series-parallel combination circuit
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
In the example circuit above, R1 and R2 are connected in a simple parallel arrangement,
as are R3 and R4. Having been identified, these sections need to be converted into equivalent
single resistors, and the circuit re-drawn:
71.429 Ω
24 V
127.27 Ω
The double slash (//) symbols represent "parallel" to show that the equivalent resistor values
were calculated using the 1/(1/R) formula. The 71.429 Ωresistor at the top of the circuit is the
equivalent of R1 and R2 in parallel with each other. The 127.27 Ωresistor at the bottom is the
202
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
equivalent of R3 and R4 in parallel with each other.
Our table can be expanded to include these resistor equivalents in their own columns:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
It should be apparent now that the circuit has been reduced to a simple series configura-
tion with only two (equivalent) resistances. The final step in reduction is to add these two
resistances to come up with a total circuit resistance. When we add those two equivalent resis-
tances, we get a resistance of 198.70 Ω. Now, we can re-draw the circuit as a single equivalent
resistance and add the total resistance figure to the rightmost column of our table. Note that
the "Total" column has been relabeled (R1//R2−−R3//R4) to indicate how it relates electrically
to the other columns of figures. The "−−" symbol is used here to represent "series," just as the
"//" symbol is used to represent "parallel."
24 V
198.70 Ω
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
Now, total circuit current can be determined by applying Ohm's Law (I=E/R) to the "Total"
column in the table:
7.2.
ANALYSIS TECHNIQUE
203
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
120.78m
Back to our equivalent circuit drawing, our total current value of 120.78 milliamps is shown
as the only current here:
I = 120.78 mA
I = 120.78 mA
24 V
198.70 Ω
Now we start to work backwards in our progression of circuit re-drawings to the original
configuration. The next step is to go to the circuit where R1//R2 and R3//R4 are in series:
I = 120.78 mA
I = 120.78 mA
I = 120.78 mA
71.429 Ω
24 V
127.27 Ω
Since R1//R2 and R3//R4 are in series with each other, the current through those two sets
of equivalent resistances must be the same. Furthermore, the current through them must be
the same as the total current, so we can fill in our table with the appropriate current values,
simply copying the current figure from the Total column to the R1//R2 and R3//R4 columns:
204
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
120.78m
120.78m
120.78m
Now, knowing the current through the equivalent resistors R1//R2 and R3//R4, we can apply
Ohm's Law (E=IR) to the two right vertical columns to find voltage drops across them:
I = 120.78 mA
I = 120.78 mA
I = 120.78 mA
71.429 Ω
127.27 Ω
24 V
8.6275 V
15.373 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
120.78m
120.78m
120.78m
8.6275
15.373
Because we know R1//R2 and R3//R4 are parallel resistor equivalents, and we know that
voltage drops in parallel circuits are the same, we can transfer the respective voltage drops to
the appropriate columns on the table for those individual resistors. In other words, we take
another step backwards in our drawing sequence to the original configuration, and complete
the table accordingly:
7.2.
ANALYSIS TECHNIQUE
205
I = 120.78 mA
I = 120.78 mA
100 Ω
350 Ω
250 Ω
200 Ω
R1
R2
R3
R4
8.6275 V
15.373 V
24 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
120.78m
120.78m
120.78m
8.6275
15.373
8.6275
8.6275
15.373
15.373
Finally, the original section of the table (columns R1 through R4) is complete with enough
values to finish. Applying Ohm's Law to the remaining vertical columns (I=E/R), we can deter-
mine the currents through R1, R2, R3, and R4 individually:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
24
100
250
350
200
71.429
127.27
198.70
120.78m
120.78m
120.78m
8.6275
15.373
8.6275
8.6275
15.373
15.373
86.275m
34.510m
43.922m
76.863m
Having found all voltage and current values for this circuit, we can show those values in
the schematic diagram as such:
206
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
I = 120.78 mA
I = 120.78 mA
43.922 mA
76.863 mA
350 Ω
200 Ω
250 Ω
100 Ω
86.275 mA
34.510 mA
R1
R2
R3
R4
8.6275 V
15.373 V
24 V
As a final check of our work, we can see if the calculated current values add up as they
should to the total. Since R1 and R2 are in parallel, their combined currents should add up
to the total of 120.78 mA. Likewise, since R3 and R4 are in parallel, their combined currents
should also add up to the total of 120.78 mA. You can check for yourself to verify that these
figures do add up as expected.
A computer simulation can also be used to verify the accuracy of these figures. The following
SPICE analysis will show all resistor voltages and currents (note the current-sensing vi1, vi2, .
. . "dummy" voltage sources in series with each resistor in the netlist, necessary for the SPICE
computer program to track current through each path). These voltage sources will be set to
have values of zero volts each so they will not affect the circuit in any way.
7.2.
ANALYSIS TECHNIQUE
207
vi1
vi2
vi3
vi4
NOTE: voltage sources vi1,
vi2, vi3, and vi4 are "dummy"
sources set at zero volts each.
1
1
1
1
2
3
4
4
4
5
6
0
0
0
0
24 V
R1
R2
R3
R4
100 Ω
250 Ω
200 Ω
350 Ω
series-parallel circuit
v1 1 0
vi1 1 2 dc 0
vi2 1 3 dc 0
r1 2 4 100
r2 3 4 250
vi3 4 5 dc 0
vi4 4 6 dc 0
r3 5 0 350
r4 6 0 200
.dc v1 24 24 1
.print dc v(2,4) v(3,4) v(5,0) v(6,0)
.print dc i(vi1) i(vi2) i(vi3) i(vi4)
.end
I've annotated SPICE's output figures to make them more readable, denoting which voltage
and current figures belong to which resistors.
v1
v(2,4)
v(3,4)
v(5)
v(6)
2.400E+01
8.627E+00
8.627E+00
1.537E+01
1.537E+01
Battery
R1 voltage
R2 voltage
R3 voltage
R4 voltage
208
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
voltage
v1
i(vi1)
i(vi2)
i(vi3)
i(vi4)
2.400E+01
8.627E-02
3.451E-02
4.392E-02
7.686E-02
Battery
R1 current
R2 current
R3 current
R4 current
voltage
As you can see, all the figures do agree with the our calculated values.
• REVIEW:
• To analyze a series-parallel combination circuit, follow these steps:
• Reduce the original circuit to a single equivalent resistor, re-drawing the circuit in each
step of reduction as simple series and simple parallel parts are reduced to single, equiv-
alent resistors.
• Solve for total resistance.
• Solve for total current (I=E/R).
• Determine equivalent resistor voltage drops and branch currents one stage at a time,
working backwards to the original circuit configuration again.
7.3
Re-drawing complex schematics
Typically, complex circuits are not arranged in nice, neat, clean schematic diagrams for us to
follow. They are often drawn in such a way that makes it difficult to follow which components
are in series and which are in parallel with each other. The purpose of this section is to show
you a method useful for re-drawing circuit schematics in a neat and orderly fashion. Like the
stage-reduction strategy for solving series-parallel combination circuits, it is a method easier
demonstrated than described.
Let's start with the following (convoluted) circuit diagram. Perhaps this diagram was orig-
inally drawn this way by a technician or engineer. Perhaps it was sketched as someone traced
the wires and connections of a real circuit. In any case, here it is in all its ugliness:
R1
R2
R3
R4
7.3.
RE-DRAWING COMPLEX SCHEMATICS
209
With electric circuits and circuit diagrams, the length and routing of wire connecting com-
ponents in a circuit matters little. (Actually, in some AC circuits it becomes critical, and very
long wire lengths can contribute unwanted resistance to both AC and DC circuits, but in most
cases wire length is irrelevant.) What this means for us is that we can lengthen, shrink, and/or
bend connecting wires without affecting the operation of our circuit.
The strategy I have found easiest to apply is to start by tracing the current from one ter-
minal of the battery around to the other terminal, following the loop of components closest to
the battery and ignoring all other wires and components for the time being. While tracing the
path of the loop, mark each resistor with the appropriate polarity for voltage drop.
In this case, I'll begin my tracing of this circuit at the negative terminal of the battery and
finish at the positive terminal, in the same general direction as the electrons would flow. When
tracing this direction, I will mark each resistor with the polarity of negative on the entering
side and positive on the exiting side, for that is how the actual polarity will be as electrons
(negative in charge) enter and exit a resistor:
Direction of electron flow
Polarity of voltage drop
R1
R2
R3
R4
Any components encountered along this short loop are drawn vertically in order:
210
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R3
Now, proceed to trace any loops of components connected around components that were just
traced. In this case, there's a loop around R1 formed by R2, and another loop around R3 formed
by R4:
R1
R3
R2
R4
loops around
R2
R1
loops around
R4
R3
Tracing those loops, I draw R2 and R4 in parallel with R1 and R3 (respectively) on the
vertical diagram. Noting the polarity of voltage drops across R3 and R1, I mark R4 and R2
likewise:
7.3.
RE-DRAWING COMPLEX SCHEMATICS
211
R1
R2
R3
R4
Now we have a circuit that is very easily understood and analyzed.
In this case, it is
identical to the four-resistor series-parallel configuration we examined earlier in the chapter.
Let's look at another example, even uglier than the one before:
R1
R2
R3
R4
R5
R6
R7
The first loop I'll trace is from the negative (-) side of the battery, through R6, through R1,
and back to the positive (+) end of the battery:
212
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
Re-drawing vertically and keeping track of voltage drop polarities along the way, our equiv-
alent circuit starts out looking like this:
R1
R6
Next, we can proceed to follow the next loop around one of the traced resistors (R6), in this
case, the loop formed by R5 and R7. As before, we start at the negative end of R6 and proceed
to the positive end of R6, marking voltage drop polarities across R7 and R5 as we go:
7.3.
RE-DRAWING COMPLEX SCHEMATICS
213
R1
R2
R3
R4
R5
R6
R7
R5 andR7
loop around
R6
Now we add the R5−−R7 loop to the vertical drawing. Notice how the voltage drop polarities
across R7 and R5 correspond with that of R6, and how this is the same as what we found tracing
R7 and R5 in the original circuit:
R1
R6
R5
R7
We repeat the process again, identifying and tracing another loop around an already-traced
resistor. In this case, the R3−−R4 loop around R5 looks like a good loop to trace next:
214
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
and
loop around
R3
R4
R5
Adding the R3−−R4 loop to the vertical drawing, marking the correct polarities as well:
R1
R6
R5
R7
R3
R4
With only one remaining resistor left to trace, then next step is obvious: trace the loop
formed by R2 around R3:
7.3.
RE-DRAWING COMPLEX SCHEMATICS
215
R1
R2
R3
R4
R5
R6
R7
loops around
R2
R3
Adding R2 to the vertical drawing, and we're finished! The result is a diagram that's very
easy to understand compared to the original:
R1
R6
R5
R7
R4
R3
R2
This simplified layout greatly eases the task of determining where to start and how to
proceed in reducing the circuit down to a single equivalent (total) resistance. Notice how the
circuit has been re-drawn, all we have to do is start from the right-hand side and work our way
left, reducing simple-series and simple-parallel resistor combinations one group at a time until
we're done.
In this particular case, we would start with the simple parallel combination of R2 and R3,
216
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
reducing it to a single resistance. Then, we would take that equivalent resistance (R2//R3) and
the one in series with it (R4), reducing them to another equivalent resistance (R2//R3−−R4).
Next, we would proceed to calculate the parallel equivalent of that resistance (R2//R3−−R4)
with R5, then in series with R7, then in parallel with R6, then in series with R1 to give us a
grand total resistance for the circuit as a whole.
From there we could calculate total current from total voltage and total resistance (I=E/R),
then "expand" the circuit back into its original form one stage at a time, distributing the ap-
propriate values of voltage and current to the resistances as we go.
• REVIEW:
• Wires in diagrams and in real circuits can be lengthened, shortened, and/or moved with-
out affecting circuit operation.
• To simplify a convoluted circuit schematic, follow these steps:
• Trace current from one side of the battery to the other, following any single path ("loop")
to the battery. Sometimes it works better to start with the loop containing the most
components, but regardless of the path taken the result will be accurate. Mark polarity
of voltage drops across each resistor as you trace the loop. Draw those components you
encounter along this loop in a vertical schematic.
• Mark traced components in the original diagram and trace remaining loops of compo-
nents in the circuit. Use polarity marks across traced components as guides for what
connects where. Document new components in loops on the vertical re-draw schematic as
well.
• Repeat last step as often as needed until all components in original diagram have been
traced.
7.4
Component failure analysis
"I consider that I understand an equation when I can predict the properties of its
solutions, without actually solving it."
P.A.M Dirac, physicist
There is a lot of truth to that quote from Dirac. With a little modification, I can extend
his wisdom to electric circuits by saying, "I consider that I understand a circuit when I can
predict the approximate effects of various changes made to it without actually performing any
calculations."
At the end of the series and parallel circuits chapter, we briefly considered how circuits
could be analyzed in a qualitative rather than quantitative manner. Building this skill is an
important step towards becoming a proficient troubleshooter of electric circuits. Once you have
a thorough understanding of how any particular failure will affect a circuit (i.e. you don't have
to perform any arithmetic to predict the results), it will be much easier to work the other way
around: pinpointing the source of trouble by assessing how a circuit is behaving.
7.4.
COMPONENT FAILURE ANALYSIS
217
Also shown at the end of the series and parallel circuits chapter was how the table method
works just as well for aiding failure analysis as it does for the analysis of healthy circuits.
We may take this technique one step further and adapt it for total qualitative analysis. By
"qualitative" I mean working with symbols representing "increase," "decrease," and "same"
instead of precise numerical figures.
We can still use the principles of series and parallel
circuits, and the concepts of Ohm's Law, we'll just use symbolic qualities instead of numerical
quantities. By doing this, we can gain more of an intuitive "feel" for how circuits work rather
than leaning on abstract equations, attaining Dirac's definition of "understanding."
Enough talk. Let's try this technique on a real circuit example and see how it works:
R1
R2
R3
R4
This is the first "convoluted" circuit we straightened out for analysis in the last section.
Since you already know how this particular circuit reduces to series and parallel sections, I'll
skip the process and go straight to the final form:
R1
R2
R3
R4
R3 and R4 are in parallel with each other; so are R1 and R2. The parallel equivalents of
R3//R4 and R1//R2 are in series with each other. Expressed in symbolic form, the total resistance
for this circuit is as follows:
First, we need to formulate a table with all the necessary rows and columns for this circuit:
218
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
Next, we need a failure scenario. Let's suppose that resistor R2 were to fail shorted. We will
assume that all other components maintain their original values. Because we'll be analyzing
this circuit qualitatively rather than quantitatively, we won't be inserting any real numbers
into the table. For any quantity unchanged after the component failure, we'll use the word
"same" to represent "no change from before." For any quantity that has changed as a result
of the failure, we'll use a down arrow for "decrease" and an up arrow for "increase." As usual,
we start by filling in the spaces of the table for individual resistances and total voltage, our
"given" values:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
The only "given" value different from the normal state of the circuit is R2, which we said
was failed shorted (abnormally low resistance). All other initial values are the same as they
were before, as represented by the "same" entries. All we have to do now is work through the
familiar Ohm's Law and series-parallel principles to determine what will happen to all the
other circuit values.
First, we need to determine what happens to the resistances of parallel subsections R1//R2
and R3//R4. If neither R3 nor R4 have changed in resistance value, then neither will their
parallel combination. However, since the resistance of R2 has decreased while R1 has stayed
the same, their parallel combination must decrease in resistance as well:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Now, we need to figure out what happens to the total resistance. This part is easy: when
we're dealing with only one component change in the circuit, the change in total resistance
will be in the same direction as the change of the failed component. This is not to say that the
magnitude of change between individual component and total circuit will be the same, merely
the direction of change. In other words, if any single resistor decreases in value, then the total
circuit resistance must also decrease, and vice versa. In this case, since R2 is the only failed
component, and its resistance has decreased, the total resistance must decrease:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Now we can apply Ohm's Law (qualitatively) to the Total column in the table. Given the fact
7.4.
COMPONENT FAILURE ANALYSIS
219
that total voltage has remained the same and total resistance has decreased, we can conclude
that total current must increase (I=E/R).
In case you're not familiar with the qualitative assessment of an equation, it works like
this. First, we write the equation as solved for the unknown quantity. In this case, we're trying
to solve for current, given voltage and resistance:
R
Now that our equation is in the proper form, we assess what change (if any) will be experi-
enced by "I," given the change(s) to "E" and "R":
R
(same)
If the denominator of a fraction decreases in value while the numerator stays the same,
then the overall value of the fraction must increase:
R
(same)
Therefore, Ohm's Law (I=E/R) tells us that the current (I) will increase. We'll mark this
conclusion in our table with an "up" arrow:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
With all resistance places filled in the table and all quantities determined in the Total
column, we can proceed to determine the other voltages and currents. Knowing that the total
resistance in this table was the result of R1//R2 and R3//R4 in series, we know that the value of
total current will be the same as that in R1//R2 and R3//R4 (because series components share the
same current). Therefore, if total current increased, then current through R1//R2 and R3//R4
must also have increased with the failure of R2:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Fundamentally, what we're doing here with a qualitative usage of Ohm's Law and the rules
of series and parallel circuits is no different from what we've done before with numerical fig-
ures. In fact, its a lot easier because you don't have to worry about making an arithmetic or
calculator keystroke error in a calculation. Instead, you're just focusing on the principles be-
hind the equations. From our table above, we can see that Ohm's Law should be applicable to
the R1//R2 and R3//R4 columns. For R3//R4, we figure what happens to the voltage, given an
increase in current and no change in resistance. Intuitively, we can see that this must result
in an increase in voltage across the parallel combination of R3//R4:
220
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
But how do we apply the same Ohm's Law formula (E=IR) to the R1//R2 column, where we
have resistance decreasing and current increasing? It's easy to determine if only one variable is
changing, as it was with R3//R4, but with two variables moving around and no definite numbers
to work with, Ohm's Law isn't going to be much help. However, there is another rule we can
apply horizontally to determine what happens to the voltage across R1//R2: the rule for voltage
in series circuits. If the voltages across R1//R2 and R3//R4 add up to equal the total (battery)
voltage and we know that the R3//R4 voltage has increased while total voltage has stayed the
same, then the voltage across R1//R2 must have decreased with the change of R2's resistance
value:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Now we're ready to proceed to some new columns in the table. Knowing that R3 and R4
comprise the parallel subsection R3//R4, and knowing that voltage is shared equally between
parallel components, the increase in voltage seen across the parallel combination R3//R4 must
also be seen across R3 and R4 individually:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
The same goes for R1 and R2. The voltage decrease seen across the parallel combination of
R1 and R2 will be seen across R1 and R2 individually:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Applying Ohm's Law vertically to those columns with unchanged ("same") resistance val-
ues, we can tell what the current will do through those components. Increased voltage across
an unchanged resistance leads to increased current. Conversely, decreased voltage across an
unchanged resistance leads to decreased current:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
Once again we find ourselves in a position where Ohm's Law can't help us: for R2, both
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
221
voltage and resistance have decreased, but without knowing how much each one has changed,
we can't use the I=E/R formula to qualitatively determine the resulting change in current.
However, we can still apply the rules of series and parallel circuits horizontally. We know that
the current through the R1//R2 parallel combination has increased, and we also know that the
current through R1 has decreased. One of the rules of parallel circuits is that total current is
equal to the sum of the individual branch currents. In this case, the current through R1//R2 is
equal to the current through R1 added to the current through R2. If current through R1//R2 has
increased while current through R1 has decreased, current through R2 must have increased:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
R4
same
same
same
same
same
And with that, our table of qualitative values stands completed. This particular exercise
may look laborious due to all the detailed commentary, but the actual process can be performed
very quickly with some practice. An important thing to realize here is that the general proce-
dure is little different from quantitative analysis: start with the known values, then proceed
to determining total resistance, then total current, then transfer figures of voltage and current
as allowed by the rules of series and parallel circuits to the appropriate columns.
A few general rules can be memorized to assist and/or to check your progress when proceed-
ing with such an analysis:
• For any single component failure (open or shorted), the total resistance will always change
in the same direction (either increase or decrease) as the resistance change of the failed
component.
• When a component fails shorted, its resistance always decreases.
Also, the current
through it will increase, and the voltage across it may drop.
I say "may" because in
some cases it will remain the same (case in point: a simple parallel circuit with an ideal
power source).
• When a component fails open, its resistance always increases. The current through that
component will decrease to zero, because it is an incomplete electrical path (no continu-
ity). This may result in an increase of voltage across it. The same exception stated above
applies here as well: in a simple parallel circuit with an ideal voltage source, the voltage
across an open-failed component will remain unchanged.
7.5
Building series-parallel resistor circuits
Once again, when building battery/resistor circuits, the student or hobbyist is faced with sev-
eral different modes of construction. Perhaps the most popular is the solderless breadboard: a
platform for constructing temporary circuits by plugging components and wires into a grid of
interconnected points. A breadboard appears to be nothing but a plastic frame with hundreds
of small holes in it. Underneath each hole, though, is a spring clip which connects to other
spring clips beneath other holes. The connection pattern between holes is simple and uniform:
222
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
Lines show common connections
underneath board between holes
Suppose we wanted to construct the following series-parallel combination circuit on a bread-
board:
R1
R2
R3
R4
100 Ω
250 Ω
200 Ω
350 Ω
24 V
A series-parallel combination circuit
The recommended way to do so on a breadboard would be to arrange the resistors in ap-
proximately the same pattern as seen in the schematic, for ease of relation to the schematic.
If 24 volts is required and we only have 6-volt batteries available, four may be connected in
series to achieve the same effect:
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
223
R1
R2
R3
R4
6 volts
6 volts
6 volts
6 volts
This is by no means the only way to connect these four resistors together to form the circuit
shown in the schematic. Consider this alternative layout:
R1
R2
R3
R4
6 volts
6 volts
6 volts
6 volts
If greater permanence is desired without resorting to soldering or wire-wrapping, one could
choose to construct this circuit on a terminal strip (also called a barrier strip, or terminal
block). In this method, components and wires are secured by mechanical tension underneath
screws or heavy clips attached to small metal bars. The metal bars, in turn, are mounted on a
nonconducting body to keep them electrically isolated from each other.
224
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
Building a circuit with components secured to a terminal strip isn't as easy as plugging
components into a breadboard, principally because the components cannot be physically ar-
ranged to resemble the schematic layout. Instead, the builder must understand how to "bend"
the schematic's representation into the real-world layout of the strip. Consider one example of
how the same four-resistor circuit could be built on a terminal strip:
6 volts
6 volts
6 volts
6 volts
R2
R3
R4
R1
Another terminal strip layout, simpler to understand and relate to the schematic, involves
anchoring parallel resistors (R1//R2 and R3//R4) to the same two terminal points on the strip
like this:
6 volts
6 volts
6 volts
6 volts
R2
R3
R4
R1
Building more complex circuits on a terminal strip involves the same spatial-reasoning
skills, but of course requires greater care and planning. Take for instance this complex circuit,
represented in schematic form:
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
225
R1
R2
R3
R4
R5
R6
R7
The terminal strip used in the prior example barely has enough terminals to mount all
seven resistors required for this circuit! It will be a challenge to determine all the necessary
wire connections between resistors, but with patience it can be done. First, begin by installing
and labeling all resistors on the strip. The original schematic diagram will be shown next to
the terminal strip circuit for reference:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Next, begin connecting components together wire by wire as shown in the schematic. Over-
draw connecting lines in the schematic to indicate completion in the real circuit. Watch this
sequence of illustrations as each individual wire is identified in the schematic, then added to
the real circuit:
226
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 1:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 2:
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
227
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 3:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 4:
228
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 5:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 6:
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
229
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 7:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 8:
230
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 9:
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 10:
7.5.
BUILDING SERIES-PARALLEL RESISTOR CIRCUITS
231
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Step 11:
Although there are minor variations possible with this terminal strip circuit, the choice of
connections shown in this example sequence is both electrically accurate (electrically identical
to the schematic diagram) and carries the additional benefit of not burdening any one screw
terminal on the strip with more than two wire ends, a good practice in any terminal strip
circuit.
An example of a "variant" wire connection might be the very last wire added (step 11), which
I placed between the left terminal of R2 and the left terminal of R3. This last wire completed
the parallel connection between R2 and R3 in the circuit. However, I could have placed this wire
instead between the left terminal of R2 and the right terminal of R1, since the right terminal
of R1 is already connected to the left terminal of R3 (having been placed there in step 9) and
so is electrically common with that one point. Doing this, though, would have resulted in three
wires secured to the right terminal of R1 instead of two, which is a faux pax in terminal strip
etiquette. Would the circuit have worked this way? Certainly! It's just that more than two
wires secured at a single terminal makes for a "messy" connection: one that is aesthetically
unpleasing and may place undue stress on the screw terminal.
Another variation would be to reverse the terminal connections for resistor R7. As shown
in the last diagram, the voltage polarity across R7 is negative on the left and positive on the
right (- , +), whereas all the other resistor polarities are positive on the left and negative on the
232
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
While this poses no electrical problem, it might cause confusion for anyone measuring resis-
tor voltage drops with a voltmeter, especially an analog voltmeter which will "peg" downscale
when subjected to a voltage of the wrong polarity. For the sake of consistency, it might be wise
to arrange all wire connections so that all resistor voltage drop polarities are the same, like
this:
7.6.
CONTRIBUTORS
233
R1
R2
R3
R4
R5
R6
R7
R1
R2
R3
R4
R5
R6
R7
Wires moved
Though electrons do not care about such consistency in component layout, people do. This
illustrates an important aspect of any engineering endeavor: the human factor. Whenever a
design may be modified for easier comprehension and/or easier maintenance - with no sacrifice
of functional performance - it should be done so.
• REVIEW:
• Circuits built on terminal strips can be difficult to lay out, but when built they are robust
enough to be considered permanent, yet easy to modify.
• It is bad practice to secure more than two wire ends and/or component leads under a
single terminal screw or clip on a terminal strip. Try to arrange connecting wires so as to
avoid this condition.
• Whenever possible, build your circuits with clarity and ease of understanding in mind.
Even though component and wiring layout is usually of little consequence in DC circuit
function, it matters significantly for the sake of the person who has to modify or trou-
bleshoot it later.
7.6
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Tony Armstrong (January 23, 2003): Suggested reversing polarity on resistor R7 in last
terminal strip circuit.
234
CHAPTER 7. SERIES-PARALLEL COMBINATION CIRCUITS
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Ron LaPlante (October 1998): helped create "table" method of series and parallel circuit
analysis.
Chapter 8
DC METERING CIRCUITS
Contents
8.1
What is a meter?
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 235
8.2
Voltmeter design
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 241
8.3
Voltmeter impact on measured circuit
. . . . . . . . . . . . . . . . . . . . . 246
8.4
Ammeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 253
8.5
Ammeter impact on measured circuit . . . . . . . . . . . . . . . . . . . . . . 260
8.6
Ohmmeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 264
8.7
High voltage ohmmeters
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 269
8.8
Multimeters
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 277
8.9
Kelvin (4-wire) resistance measurement
. . . . . . . . . . . . . . . . . . . . 281
8.10 Bridge circuits . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 288
8.11 Wattmeter design . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 295
8.12 Creating custom calibration resistances . . . . . . . . . . . . . . . . . . . . 296
8.13 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 299
8.1
What is a meter?
A meter is any device built to accurately detect and display an electrical quantity in a form
readable by a human being. Usually this "readable form" is visual: motion of a pointer on
a scale, a series of lights arranged to form a "bargraph," or some sort of display composed
of numerical figures.
In the analysis and testing of circuits, there are meters designed to
accurately measure the basic quantities of voltage, current, and resistance. There are many
other types of meters as well, but this chapter primarily covers the design and operation of the
basic three.
Most modern meters are "digital" in design, meaning that their readable display is in the
form of numerical digits. Older designs of meters are mechanical in nature, using some kind
of pointer device to show quantity of measurement. In either case, the principles applied in
235
236
CHAPTER 8. DC METERING CIRCUITS
adapting a display unit to the measurement of (relatively) large quantities of voltage, current,
or resistance are the same.
The display mechanism of a meter is often referred to as a movement, borrowing from its
mechanical nature to move a pointer along a scale so that a measured value may be read.
Though modern digital meters have no moving parts, the term "movement" may be applied to
the same basic device performing the display function.
The design of digital "movements" is beyond the scope of this chapter, but mechanical meter
movement designs are very understandable. Most mechanical movements are based on the
principle of electromagnetism: that electric current through a conductor produces a magnetic
field perpendicular to the axis of electron flow. The greater the electric current, the stronger the
magnetic field produced. If the magnetic field formed by the conductor is allowed to interact
with another magnetic field, a physical force will be generated between the two sources of
fields. If one of these sources is free to move with respect to the other, it will do so as current
is conducted through the wire, the motion (usually against the resistance of a spring) being
proportional to strength of current.
The first meter movements built were known as galvanometers, and were usually designed
with maximum sensitivity in mind. A very simple galvanometer may be made from a mag-
netized needle (such as the needle from a magnetic compass) suspended from a string, and
positioned within a coil of wire. Current through the wire coil will produce a magnetic field
which will deflect the needle from pointing in the direction of earth's magnetic field. An antique
string galvanometer is shown in the following photograph:
Such instruments were useful in their time, but have little place in the modern world ex-
cept as proof-of-concept and elementary experimental devices. They are highly susceptible to
motion of any kind, and to any disturbances in the natural magnetic field of the earth. Now,
the term "galvanometer" usually refers to any design of electromagnetic meter movement built
for exceptional sensitivity, and not necessarily a crude device such as that shown in the pho-
tograph. Practical electromagnetic meter movements can be made now where a pivoting wire
coil is suspended in a strong magnetic field, shielded from the majority of outside influences.
Such an instrument design is generally known as a permanent-magnet, moving coil, or PMMC
movement:
8.1.
WHAT IS A METER?
237
wire coil
meter terminal
connections
magnet
magnet
"needle"
0
50
100
current through wire coil
causes needle to deflect
Permanent magnet, moving coil (PMMC) meter movement
In the picture above, the meter movement "needle" is shown pointing somewhere around
35 percent of full-scale, zero being full to the left of the arc and full-scale being completely to
the right of the arc. An increase in measured current will drive the needle to point further
to the right and a decrease will cause the needle to drop back down toward its resting point
on the left. The arc on the meter display is labeled with numbers to indicate the value of the
quantity being measured, whatever that quantity is. In other words, if it takes 50 microamps
of current to drive the needle fully to the right (making this a "50 μA full-scale movement"),
the scale would have 0 μA written at the very left end and 50 μA at the very right, 25 μA
being marked in the middle of the scale. In all likelihood, the scale would be divided into much
smaller graduating marks, probably every 5 or 1 μA, to allow whoever is viewing the movement
to infer a more precise reading from the needle's position.
The meter movement will have a pair of metal connection terminals on the back for current
to enter and exit. Most meter movements are polarity-sensitive, one direction of current driv-
ing the needle to the right and the other driving it to the left. Some meter movements have
a needle that is spring-centered in the middle of the scale sweep instead of to the left, thus
enabling measurements of either polarity:
238
CHAPTER 8. DC METERING CIRCUITS
0
100
A "zero-center" meter movement
Common polarity-sensitive movements include the D'Arsonval and Weston designs, both
PMMC-type instruments. Current in one direction through the wire will produce a clockwise
torque on the needle mechanism, while current the other direction will produce a counter-
clockwise torque.
Some meter movements are polarity-insensitive, relying on the attraction of an unmagne-
tized, movable iron vane toward a stationary, current-carrying wire to deflect the needle. Such
meters are ideally suited for the measurement of alternating current (AC). A polarity-sensitive
movement would just vibrate back and forth uselessly if connected to a source of AC.
While most mechanical meter movements are based on electromagnetism (electron flow
through a conductor creating a perpendicular magnetic field), a few are based on electrostatics:
that is, the attractive or repulsive force generated by electric charges across space. This is
the same phenomenon exhibited by certain materials (such as wax and wool) when rubbed
together. If a voltage is applied between two conductive surfaces across an air gap, there will be
a physical force attracting the two surfaces together capable of moving some kind of indicating
mechanism. That physical force is directly proportional to the voltage applied between the
plates, and inversely proportional to the square of the distance between the plates. The force
is also irrespective of polarity, making this a polarity-insensitive type of meter movement:
8.1.
WHAT IS A METER?
239
force
Voltage to be measured
Electrostatic meter movement
Unfortunately, the force generated by the electrostatic attraction is very small for common
voltages. In fact, it is so small that such meter movement designs are impractical for use in
general test instruments. Typically, electrostatic meter movements are used for measuring
very high voltages (many thousands of volts). One great advantage of the electrostatic meter
movement, however, is the fact that it has extremely high resistance, whereas electromagnetic
movements (which depend on the flow of electrons through wire to generate a magnetic field)
are much lower in resistance. As we will see in greater detail to come, greater resistance
(resulting in less current drawn from the circuit under test) makes for a better voltmeter.
A much more common application of electrostatic voltage measurement is seen in an device
known as a Cathode Ray Tube, or CRT. These are special glass tubes, very similar to tele-
vision viewscreen tubes. In the cathode ray tube, a beam of electrons traveling in a vacuum
are deflected from their course by voltage between pairs of metal plates on either side of the
beam. Because electrons are negatively charged, they tend to be repelled by the negative plate
and attracted to the positive plate. A reversal of voltage polarity across the two plates will
result in a deflection of the electron beam in the opposite direction, making this type of meter
"movement" polarity-sensitive:
electron "gun"
electrons
plates
voltage to be measured
electrons
light
screen
(vacuum)
The electrons, having much less mass than metal plates, are moved by this electrostatic
force very quickly and readily. Their deflected path can be traced as the electrons impinge on
the glass end of the tube where they strike a coating of phosphorus chemical, emitting a glow
of light seen outside of the tube. The greater the voltage between the deflection plates, the
240
CHAPTER 8. DC METERING CIRCUITS
further the electron beam will be "bent" from its straight path, and the further the glowing
spot will be seen from center on the end of the tube.
A photograph of a CRT is shown here:
In a real CRT, as shown in the above photograph, there are two pairs of deflection plates
rather than just one. In order to be able to sweep the electron beam around the whole area
of the screen rather than just in a straight line, the beam must be deflected in more than one
dimension.
Although these tubes are able to accurately register small voltages, they are bulky and
require electrical power to operate (unlike electromagnetic meter movements, which are more
compact and actuated by the power of the measured signal current going through them). They
are also much more fragile than other types of electrical metering devices. Usually, cathode
ray tubes are used in conjunction with precise external circuits to form a larger piece of test
equipment known as an oscilloscope, which has the ability to display a graph of voltage over
time, a tremendously useful tool for certain types of circuits where voltage and/or current levels
are dynamically changing.
Whatever the type of meter or size of meter movement, there will be a rated value of voltage
or current necessary to give full-scale indication. In electromagnetic movements, this will be
the "full-scale deflection current" necessary to rotate the needle so that it points to the exact
end of the indicating scale. In electrostatic movements, the full-scale rating will be expressed
as the value of voltage resulting in the maximum deflection of the needle actuated by the
plates, or the value of voltage in a cathode-ray tube which deflects the electron beam to the
edge of the indicating screen. In digital "movements," it is the amount of voltage resulting
in a "full-count" indication on the numerical display: when the digits cannot display a larger
quantity.
The task of the meter designer is to take a given meter movement and design the necessary
external circuitry for full-scale indication at some specified amount of voltage or current. Most
meter movements (electrostatic movements excepted) are quite sensitive, giving full-scale indi-
cation at only a small fraction of a volt or an amp. This is impractical for most tasks of voltage
and current measurement. What the technician often requires is a meter capable of measuring
high voltages and currents.
By making the sensitive meter movement part of a voltage or current divider circuit, the
movement's useful measurement range may be extended to measure far greater levels than
what could be indicated by the movement alone. Precision resistors are used to create the
divider circuits necessary to divide voltage or current appropriately. One of the lessons you
will learn in this chapter is how to design these divider circuits.
• REVIEW:
• A "movement" is the display mechanism of a meter.
8.2.
VOLTMETER DESIGN
241
• Electromagnetic movements work on the principle of a magnetic field being generated by
electric current through a wire. Examples of electromagnetic meter movements include
the D'Arsonval, Weston, and iron-vane designs.
• Electrostatic movements work on the principle of physical force generated by an electric
field between two plates.
• Cathode Ray Tubes (CRT's) use an electrostatic field to bend the path of an electron beam,
providing indication of the beam's position by light created when the beam strikes the end
of the glass tube.
8.2
Voltmeter design
As was stated earlier, most meter movements are sensitive devices. Some D'Arsonval move-
ments have full-scale deflection current ratings as little as 50 μA, with an (internal) wire re-
sistance of less than 1000 Ω. This makes for a voltmeter with a full-scale rating of only 50
millivolts (50 μA X 1000 Ω)! In order to build voltmeters with practical (higher voltage) scales
from such sensitive movements, we need to find some way to reduce the measured quantity of
voltage down to a level the movement can handle.
Let's start our example problems with a D'Arsonval meter movement having a full-scale
deflection rating of 1 mA and a coil resistance of 500 Ω:
black test
lead
lead
red test
500 Ω
Using Ohm's Law (E=IR), we can determine how much voltage will drive this meter move-
ment directly to full scale:
E = (1 mA)(500 Ω)
E = 0.5 volts
If all we wanted was a meter that could measure 1/2 of a volt, the bare meter movement we
have here would suffice. But to measure greater levels of voltage, something more is needed.
To get an effective voltmeter meter range in excess of 1/2 volt, we'll need to design a circuit
allowing only a precise proportion of measured voltage to drop across the meter movement.
242
CHAPTER 8. DC METERING CIRCUITS
This will extend the meter movement's range to higher voltages. Correspondingly, we will
need to re-label the scale on the meter face to indicate its new measurement range with this
proportioning circuit connected.
But how do we create the necessary proportioning circuit? Well, if our intention is to allow
this meter movement to measure a greater voltage than it does now, what we need is a voltage
divider circuit to proportion the total measured voltage into a lesser fraction across the meter
movement's connection points. Knowing that voltage divider circuits are built from series re-
sistances, we'll connect a resistor in series with the meter movement (using the movement's
own internal resistance as the second resistance in the divider):
black test
lead
lead
red test
500 Ω
Rmultiplier
The series resistor is called a "multiplier" resistor because it multiplies the working range of
the meter movement as it proportionately divides the measured voltage across it. Determining
the required multiplier resistance value is an easy task if you're familiar with series circuit
analysis.
For example, let's determine the necessary multiplier value to make this 1 mA, 500 Ωmove-
ment read exactly full-scale at an applied voltage of 10 volts. To do this, we first need to set up
an E/I/R table for the two series components:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rmultiplier
Knowing that the movement will be at full-scale with 1 mA of current going through it, and
that we want this to happen at an applied (total series circuit) voltage of 10 volts, we can fill in
the table as such:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rmultiplier
10
1m
1m
1m
500
There are a couple of ways to determine the resistance value of the multiplier. One way
is to determine total circuit resistance using Ohm's Law in the "total" column (R=E/I), then
8.2.
VOLTMETER DESIGN
243
subtract the 500 Ωof the movement to arrive at the value for the multiplier:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rmultiplier
10
1m
1m
1m
500
10k
9.5k
Another way to figure the same value of resistance would be to determine voltage drop
across the movement at full-scale deflection (E=IR), then subtract that voltage drop from the
total to arrive at the voltage across the multiplier resistor. Finally, Ohm's Law could be used
again to determine resistance (R=E/I) for the multiplier:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rmultiplier
10
1m
1m
1m
500
10k
9.5k
0.5
9.5
Either way provides the same answer (9.5 kΩ), and one method could be used as verification
for the other, to check accuracy of work.
black test
lead
lead
red test
10 volts gives full-scale
deflection of needle
9.5 kΩ
500 Ω F.S. = 1 mA
Rmultiplier
10 V
Meter movement ranged for 10 volts full-scale
With exactly 10 volts applied between the meter test leads (from some battery or precision
power supply), there will be exactly 1 mA of current through the meter movement, as restricted
by the "multiplier" resistor and the movement's own internal resistance. Exactly 1/2 volt will
be dropped across the resistance of the movement's wire coil, and the needle will be pointing
precisely at full-scale. Having re-labeled the scale to read from 0 to 10 V (instead of 0 to 1 mA),
anyone viewing the scale will interpret its indication as ten volts. Please take note that the
meter user does not have to be aware at all that the movement itself is actually measuring just
244
CHAPTER 8. DC METERING CIRCUITS
a fraction of that ten volts from the external source. All that matters to the user is that the
circuit as a whole functions to accurately display the total, applied voltage.
This is how practical electrical meters are designed and used: a sensitive meter movement
is built to operate with as little voltage and current as possible for maximum sensitivity, then
it is "fooled" by some sort of divider circuit built of precision resistors so that it indicates full-
scale when a much larger voltage or current is impressed on the circuit as a whole. We have
examined the design of a simple voltmeter here. Ammeters follow the same general rule, except
that parallel-connected "shunt" resistors are used to create a current divider circuit as opposed
to the series-connected voltage divider "multiplier" resistors used for voltmeter designs.
Generally, it is useful to have multiple ranges established for an electromechanical meter
such as this, allowing it to read a broad range of voltages with a single movement mechanism.
This is accomplished through the use of a multi-pole switch and several multiplier resistors,
each one sized for a particular voltage range:
black test
lead
lead
red test
range selector
switch
500 Ω F.S. = 1 mA
R1
R2
R3
R4
A multi-range voltmeter
The five-position switch makes contact with only one resistor at a time. In the bottom (full
clockwise) position, it makes contact with no resistor at all, providing an "off" setting. Each
resistor is sized to provide a particular full-scale range for the voltmeter, all based on the
particular rating of the meter movement (1 mA, 500 Ω). The end result is a voltmeter with
four different full-scale ranges of measurement. Of course, in order to make this work sensibly,
the meter movement's scale must be equipped with labels appropriate for each range.
With such a meter design, each resistor value is determined by the same technique, using
a known total voltage, movement full-scale deflection rating, and movement resistance. For a
voltmeter with ranges of 1 volt, 10 volts, 100 volts, and 1000 volts, the multiplier resistances
would be as follows:
8.2.
VOLTMETER DESIGN
245
black test
lead
lead
red test
range selector
switch
off
R1 = 999.5 kΩ
R2 = 99.5 kΩ
500 Ω F.S. = 1 mA
R1
R2
R3
R4
1000 V
100 V
10 V
1 V
Note the multiplier resistor values used for these ranges, and how odd they are. It is highly
unlikely that a 999.5 kΩprecision resistor will ever be found in a parts bin, so voltmeter
designers often opt for a variation of the above design which uses more common resistor values:
black test
lead
lead
red test
range selector
switch
off
R1 = 900 kΩ
500 Ω F.S. = 1 mA
R1
R2
R3
R4
1000 V
100 V
10 V
1 V
With each successively higher voltage range, more multiplier resistors are pressed into
service by the selector switch, making their series resistances add for the necessary total. For
example, with the range selector switch set to the 1000 volt position, we need a total multiplier
resistance value of 999.5 kΩ. With this meter design, that's exactly what we'll get:
RT otal = 999.5 kΩ
The advantage, of course, is that the individual multiplier resistor values are more common
(900k, 90k, 9k) than some of the odd values in the first design (999.5k, 99.5k, 9.5k). From the
perspective of the meter user, however, there will be no discernible difference in function.
246
CHAPTER 8. DC METERING CIRCUITS
• REVIEW:
• Extended voltmeter ranges are created for sensitive meter movements by adding series
"multiplier" resistors to the movement circuit, providing a precise voltage division ratio.
8.3
Voltmeter impact on measured circuit
Every meter impacts the circuit it is measuring to some extent, just as any tire-pressure gauge
changes the measured tire pressure slightly as some air is let out to operate the gauge. While
some impact is inevitable, it can be minimized through good meter design.
Since voltmeters are always connected in parallel with the component or components un-
der test, any current through the voltmeter will contribute to the overall current in the tested
circuit, potentially affecting the voltage being measured. A perfect voltmeter has infinite re-
sistance, so that it draws no current from the circuit under test. However, perfect voltmeters
only exist in the pages of textbooks, not in real life! Take the following voltage divider circuit
as an extreme example of how a realistic voltmeter might impact the circuit its measuring:
V
voltmeter
250 MΩ
250 MΩ
24 V
With no voltmeter connected to the circuit, there should be exactly 12 volts across each 250
MΩresistor in the series circuit, the two equal-value resistors dividing the total voltage (24
volts) exactly in half. However, if the voltmeter in question has a lead-to-lead resistance of
10 MΩ(a common amount for a modern digital voltmeter), its resistance will create a parallel
subcircuit with the lower resistor of the divider when connected:
8.3.
VOLTMETER IMPACT ON MEASURED CIRCUIT
247
V
voltmeter
250 MΩ
250 MΩ
(10 MΩ)
24 V
This effectively reduces the lower resistance from 250 MΩto 9.615 MΩ(250 MΩand 10 MΩ
in parallel), drastically altering voltage drops in the circuit. The lower resistor will now have
far less voltage across it than before, and the upper resistor far more.
250 MΩ
9.615 MΩ
24 V
0.8889 V
23.1111 V
A voltage divider with resistance values of 250 MΩand 9.615 MΩwill divide 24 volts into
portions of 23.1111 volts and 0.8889 volts, respectively. Since the voltmeter is part of that
9.615 MΩresistance, that is what it will indicate: 0.8889 volts.
Now, the voltmeter can only indicate the voltage its connected across. It has no way of
"knowing" there was a potential of 12 volts dropped across the lower 250 MΩresistor before it
was connected across it. The very act of connecting the voltmeter to the circuit makes it part of
the circuit, and the voltmeter's own resistance alters the resistance ratio of the voltage divider
circuit, consequently affecting the voltage being measured.
Imagine using a tire pressure gauge that took so great a volume of air to operate that it
would deflate any tire it was connected to. The amount of air consumed by the pressure gauge
in the act of measurement is analogous to the current taken by the voltmeter movement to
move the needle. The less air a pressure gauge requires to operate, the less it will deflate the
tire under test. The less current drawn by a voltmeter to actuate the needle, the less it will
burden the circuit under test.
This effect is called loading, and it is present to some degree in every instance of voltmeter
usage. The scenario shown here is worst-case, with a voltmeter resistance substantially lower
248
CHAPTER 8. DC METERING CIRCUITS
than the resistances of the divider resistors. But there always will be some degree of loading,
causing the meter to indicate less than the true voltage with no meter connected. Obviously,
the higher the voltmeter resistance, the less loading of the circuit under test, and that is why
an ideal voltmeter has infinite internal resistance.
Voltmeters with electromechanical movements are typically given ratings in "ohms per volt"
of range to designate the amount of circuit impact created by the current draw of the move-
ment. Because such meters rely on different values of multiplier resistors to give different mea-
surement ranges, their lead-to-lead resistances will change depending on what range they're
set to. Digital voltmeters, on the other hand, often exhibit a constant resistance across their
test leads regardless of range setting (but not always!), and as such are usually rated simply
in ohms of input resistance, rather than "ohms per volt" sensitivity.
What "ohms per volt" means is how many ohms of lead-to-lead resistance for every volt of
range setting on the selector switch. Let's take our example voltmeter from the last section as
an example:
black test
lead
lead
red test
range selector
switch
off
R1 = 999.5 kΩ
R2 = 99.5 kΩ
500 Ω F.S. = 1 mA
R1
R2
R3
R4
1000 V
100 V
10 V
1 V
On the 1000 volt scale, the total resistance is 1 MΩ(999.5 kΩ+ 500Ω), giving 1,000,000 Ω
per 1000 volts of range, or 1000 ohms per volt (1 kΩ/V). This ohms-per-volt "sensitivity" rating
remains constant for any range of this meter:
100 volt range
100 kΩ
100 V
= 1000 Ω/V sensitivity
= 1000 Ω/V sensitivity
10 kΩ
10 V
10 volt range
= 1000 Ω/V sensitivity
1 kΩ
1 V
1 volt range
The astute observer will notice that the ohms-per-volt rating of any meter is determined by
a single factor: the full-scale current of the movement, in this case 1 mA. "Ohms per volt" is the
mathematical reciprocal of "volts per ohm," which is defined by Ohm's Law as current (I=E/R).
Consequently, the full-scale current of the movement dictates the Ω/volt sensitivity of the meter,
8.3.
VOLTMETER IMPACT ON MEASURED CIRCUIT
249
regardless of what ranges the designer equips it with through multiplier resistors. In this case,
the meter movement's full-scale current rating of 1 mA gives it a voltmeter sensitivity of 1000
Ω/V regardless of how we range it with multiplier resistors.
To minimize the loading of a voltmeter on any circuit, the designer must seek to minimize
the current draw of its movement. This can be accomplished by re-designing the movement
itself for maximum sensitivity (less current required for full-scale deflection), but the tradeoff
here is typically ruggedness: a more sensitive movement tends to be more fragile.
Another approach is to electronically boost the current sent to the movement, so that very
little current needs to be drawn from the circuit under test. This special electronic circuit is
known as an amplifier, and the voltmeter thus constructed is an amplified voltmeter.
Amplifier
Battery
red test
lead
black test
lead
Amplified voltmeter
The internal workings of an amplifier are too complex to be discussed at this point, but suf-
fice it to say that the circuit allows the measured voltage to control how much battery current
is sent to the meter movement. Thus, the movement's current needs are supplied by a battery
internal to the voltmeter and not by the circuit under test. The amplifier still loads the circuit
under test to some degree, but generally hundreds or thousands of times less than the meter
movement would by itself.
Before the advent of semiconductors known as "field-effect transistors," vacuum tubes were
used as amplifying devices to perform this boosting. Such vacuum-tube voltmeters, or (VTVM's)
were once very popular instruments for electronic test and measurement. Here is a photograph
of a very old VTVM, with the vacuum tube exposed!
Now, solid-state transistor amplifier circuits accomplish the same task in digital meter de-
signs. While this approach (of using an amplifier to boost the measured signal current) works
250
CHAPTER 8. DC METERING CIRCUITS
well, it vastly complicates the design of the meter, making it nearly impossible for the begin-
ning electronics student to comprehend its internal workings.
A final, and ingenious, solution to the problem of voltmeter loading is that of the potentio-
metric or null-balance instrument. It requires no advanced (electronic) circuitry or sensitive
devices like transistors or vacuum tubes, but it does require greater technician involvement
and skill. In a potentiometric instrument, a precision adjustable voltage source is compared
against the measured voltage, and a sensitive device called a null detector is used to indi-
cate when the two voltages are equal. In some circuit designs, a precision potentiometer is
used to provide the adjustable voltage, hence the label potentiometric. When the voltages are
equal, there will be zero current drawn from the circuit under test, and thus the measured
voltage should be unaffected. It is easy to show how this works with our last example, the
high-resistance voltage divider circuit:
adjustable
voltage
source
1
2
250 MΩ
250 MΩ
R1
R2
24 V
Potentiometric voltage measurement
"null" detector
null
The "null detector" is a sensitive device capable of indicating the presence of very small
voltages. If an electromechanical meter movement is used as the null detector, it will have a
spring-centered needle that can deflect in either direction so as to be useful for indicating a
voltage of either polarity. As the purpose of a null detector is to accurately indicate a condition
of zero voltage, rather than to indicate any specific (nonzero) quantity as a normal voltmeter
would, the scale of the instrument used is irrelevant. Null detectors are typically designed to
be as sensitive as possible in order to more precisely indicate a "null" or "balance" (zero voltage)
condition.
An extremely simple type of null detector is a set of audio headphones, the speakers within
acting as a kind of meter movement. When a DC voltage is initially applied to a speaker, the
resulting current through it will move the speaker cone and produce an audible "click." Another
"click" sound will be heard when the DC source is disconnected. Building on this principle, a
sensitive null detector may be made from nothing more than headphones and a momentary
contact switch:
8.3.
VOLTMETER IMPACT ON MEASURED CIRCUIT
251
Headphones
Test
leads
Pushbutton
switch
If a set of "8 ohm" headphones are used for this purpose, its sensitivity may be greatly
increased by connecting it to a device called a transformer. The transformer exploits principles
of electromagnetism to "transform" the voltage and current levels of electrical energy pulses.
In this case, the type of transformer used is a step-down transformer, and it converts low-
current pulses (created by closing and opening the pushbutton switch while connected to a
small voltage source) into higher-current pulses to more efficiently drive the speaker cones
inside the headphones. An "audio output" transformer with an impedance ratio of 1000:8 is
ideal for this purpose. The transformer also increases detector sensitivity by accumulating
the energy of a low-current signal in a magnetic field for sudden release into the headphone
speakers when the switch is opened. Thus, it will produce louder "clicks" for detecting smaller
signals:
Headphones
Test
leads
1 kΩ
8 Ω
Audio output
transformer
Connected to the potentiometric circuit as a null detector, the switch/transformer/headphone
arrangement is used as such:
252
CHAPTER 8. DC METERING CIRCUITS
adjustable
voltage
source
1
2
250 MΩ
250 MΩ
R1
R2
24 V
Push button to
test for balance
The purpose of any null detector is to act like a laboratory balance scale, indicating when
the two voltages are equal (absence of voltage between points 1 and 2) and nothing more.
The laboratory scale balance beam doesn't actually weigh anything; rather, it simply indicates
equality between the unknown mass and the pile of standard (calibrated) masses.
x
mass standards
unknown mass
Likewise, the null detector simply indicates when the voltage between points 1 and 2 are
equal, which (according to Kirchhoff's Voltage Law) will be when the adjustable voltage source
(the battery symbol with a diagonal arrow going through it) is precisely equal in voltage to the
drop across R2.
To operate this instrument, the technician would manually adjust the output of the preci-
sion voltage source until the null detector indicated exactly zero (if using audio headphones
as the null detector, the technician would repeatedly press and release the pushbutton switch,
listening for silence to indicate that the circuit was "balanced"), and then note the source volt-
age as indicated by a voltmeter connected across the precision voltage source, that indication
being representative of the voltage across the lower 250 MΩresistor:
8.4.
AMMETER DESIGN
253
adjustable
voltage
source
1
2
V
250 MΩ
250 MΩ
R1
R2
24 V
null
"null" detector
Adjust voltage source until null detector registers zero.
Then, read voltmeter indication for voltage across R2.
The voltmeter used to directly measure the precision source need not have an extremely
high Ω/V sensitivity, because the source will supply all the current it needs to operate. So long
as there is zero voltage across the null detector, there will be zero current between points 1 and
2, equating to no loading of the divider circuit under test.
It is worthy to reiterate the fact that this method, properly executed, places almost zero
load upon the measured circuit. Ideally, it places absolutely no load on the tested circuit, but
to achieve this ideal goal the null detector would have to have absolutely zero voltage across
it, which would require an infinitely sensitive null meter and a perfect balance of voltage from
the adjustable voltage source. However, despite its practical inability to achieve absolute zero
loading, a potentiometric circuit is still an excellent technique for measuring voltage in high-
resistance circuits. And unlike the electronic amplifier solution, which solves the problem with
advanced technology, the potentiometric method achieves a hypothetically perfect solution by
exploiting a fundamental law of electricity (KVL).
• REVIEW:
• An ideal voltmeter has infinite resistance.
• Too low of an internal resistance in a voltmeter will adversely affect the circuit being
measured.
• Vacuum tube voltmeters (VTVM's), transistor voltmeters, and potentiometric circuits are
all means of minimizing the load placed on a measured circuit. Of these methods, the
potentiometric ("null-balance") technique is the only one capable of placing zero load on
the circuit.
• A null detector is a device built for maximum sensitivity to small voltages or currents.
It is used in potentiometric voltmeter circuits to indicate the absence of voltage between
two points, thus indicating a condition of balance between an adjustable voltage source
and the voltage being measured.
8.4
Ammeter design
A meter designed to measure electrical current is popularly called an "ammeter" because the
unit of measurement is "amps."
254
CHAPTER 8. DC METERING CIRCUITS
In ammeter designs, external resistors added to extend the usable range of the movement
are connected in parallel with the movement rather than in series as is the case for voltmeters.
This is because we want to divide the measured current, not the measured voltage, going to
the movement, and because current divider circuits are always formed by parallel resistances.
Taking the same meter movement as the voltmeter example, we can see that it would make
a very limited instrument by itself, full-scale deflection occurring at only 1 mA:
As is the case with extending a meter movement's voltage-measuring ability, we would have
to correspondingly re-label the movement's scale so that it read differently for an extended
current range. For example, if we wanted to design an ammeter to have a full-scale range of
5 amps using the same meter movement as before (having an intrinsic full-scale range of only
1 mA), we would have to re-label the movement's scale to read 0 A on the far left and 5 A on
the far right, rather than 0 mA to 1 mA as before. Whatever extended range provided by the
parallel-connected resistors, we would have to represent graphically on the meter movement
face.
black test
lead
lead
red test
500 Ω
Using 5 amps as an extended range for our sample movement, let's determine the amount
of parallel resistance necessary to "shunt," or bypass, the majority of current so that only 1 mA
will go through the movement with a total current of 5 A:
black test
lead
lead
red test
500 Ω F.S. = 1 mA
Rshunt
8.4.
AMMETER DESIGN
255
E
I
R
Volts
Amps
Ohms
Total
Movement
Rshunt
5
1m
500
From our given values of movement current, movement resistance, and total circuit (mea-
sured) current, we can determine the voltage across the meter movement (Ohm's Law applied
to the center column, E=IR):
E
I
R
Volts
Amps
Ohms
Total
Movement
Rshunt
5
1m
500
0.5
Knowing that the circuit formed by the movement and the shunt is of a parallel configura-
tion, we know that the voltage across the movement, shunt, and test leads (total) must be the
same:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rshunt
5
1m
500
0.5
0.5
0.5
We also know that the current through the shunt must be the difference between the total
current (5 amps) and the current through the movement (1 mA), because branch currents add
in a parallel configuration:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rshunt
5
1m
500
0.5
0.5
0.5
4.999
Then, using Ohm's Law (R=E/I) in the right column, we can determine the necessary shunt
resistance:
E
I
R
Volts
Amps
Ohms
Total
Movement
Rshunt
5
1m
500
0.5
0.5
0.5
4.999
100.02m
Of course, we could have calculated the same value of just over 100 milli-ohms (100 mΩ)
for the shunt by calculating total resistance (R=E/I; 0.5 volts/5 amps = 100 mΩexactly), then
256
CHAPTER 8. DC METERING CIRCUITS
working the parallel resistance formula backwards, but the arithmetic would have been more
challenging:
1
1
1
100m
500
Rshunt = 100.02 mΩ
In real life, the shunt resistor of an ammeter will usually be encased within the protective
metal housing of the meter unit, hidden from sight. Note the construction of the ammeter in
the following photograph:
This particular ammeter is an automotive unit manufactured by Stewart-Warner. Although
the D'Arsonval meter movement itself probably has a full scale rating in the range of mil-
liamps, the meter as a whole has a range of +/- 60 amps. The shunt resistor providing this
high current range is enclosed within the metal housing of the meter. Note also with this par-
ticular meter that the needle centers at zero amps and can indicate either a "positive" current
or a "negative" current. Connected to the battery charging circuit of an automobile, this me-
ter is able to indicate a charging condition (electrons flowing from generator to battery) or a
discharging condition (electrons flowing from battery to the rest of the car's loads).
As is the case with multiple-range voltmeters, ammeters can be given more than one usable
range by incorporating several shunt resistors switched with a multi-pole switch:
8.4.
AMMETER DESIGN
257
black test
lead
lead
red test
range selector
switch
500 Ω F.S. = 1 mA
R1
R2
R3
R4
A multirange ammeter
off
Notice that the range resistors are connected through the switch so as to be in parallel
with the meter movement, rather than in series as it was in the voltmeter design. The five-
position switch makes contact with only one resistor at a time, of course. Each resistor is
sized accordingly for a different full-scale range, based on the particular rating of the meter
movement (1 mA, 500 Ω).
With such a meter design, each resistor value is determined by the same technique, using a
known total current, movement full-scale deflection rating, and movement resistance. For an
ammeter with ranges of 100 mA, 1 A, 10 A, and 100 A, the shunt resistances would be as such:
black test
lead
lead
red test
range selector
switch
100 A
10 A
1 A
100 mA
off
500 Ω F.S. = 1 mA
R1
R2
R3
R4
R1 = 5.00005 mΩ
R2 = 50.005 mΩ
R3 = 500.5005 mΩ
R4 = 5.05051 Ω
Notice that these shunt resistor values are very low! 5.00005 mΩis 5.00005 milli-ohms, or
0.00500005 ohms! To achieve these low resistances, ammeter shunt resistors often have to be
custom-made from relatively large-diameter wire or solid pieces of metal.
One thing to be aware of when sizing ammeter shunt resistors is the factor of power dis-
sipation. Unlike the voltmeter, an ammeter's range resistors have to carry large amounts of
current. If those shunt resistors are not sized accordingly, they may overheat and suffer dam-
age, or at the very least lose accuracy due to overheating. For the example meter above, the
258
CHAPTER 8. DC METERING CIRCUITS
power dissipations at full-scale indication are (the double-squiggly lines represent "approxi-
mately equal to" in mathematics):
R1
5.00005 mΩ
(0.5 V)2
50 W
E2
(0.5 V)2
R2
50.005 mΩ
5 W
E2
(0.5 V)2
E2
(0.5 V)2
R3
R4
500.5 mΩ
0.5 W
5.05 Ω
49.5 mW
An 1/8 watt resistor would work just fine for R4, a 1/2 watt resistor would suffice for R3
and a 5 watt for R2 (although resistors tend to maintain their long-term accuracy better if
not operated near their rated power dissipation, so you might want to over-rate resistors R2
and R3), but precision 50 watt resistors are rare and expensive components indeed. A custom
resistor made from metal stock or thick wire may have to be constructed for R1 to meet both
the requirements of low resistance and high power rating.
Sometimes, shunt resistors are used in conjunction with voltmeters of high input resistance
to measure current. In these cases, the current through the voltmeter movement is small
enough to be considered negligible, and the shunt resistance can be sized according to how
many volts or millivolts of drop will be produced per amp of current:
V
current to be
measured
measured
current to be
voltmeter
Rshunt
If, for example, the shunt resistor in the above circuit were sized at precisely 1 Ω, there
would be 1 volt dropped across it for every amp of current through it. The voltmeter indication
could then be taken as a direct indication of current through the shunt. For measuring very
small currents, higher values of shunt resistance could be used to generate more voltage drop
8.4.
AMMETER DESIGN
259
per given unit of current, thus extending the usable range of the (volt)meter down into lower
amounts of current. The use of voltmeters in conjunction with low-value shunt resistances for
the measurement of current is something commonly seen in industrial applications.
The use of a shunt resistor along with a voltmeter to measure current can be a useful trick
for simplifying the task of frequent current measurements in a circuit. Normally, to measure
current through a circuit with an ammeter, the circuit would have to be broken (interrupted)
and the ammeter inserted between the separated wire ends, like this:
Load
A
If we have a circuit where current needs to be measured often, or we would just like to
make the process of current measurement more convenient, a shunt resistor could be placed
between those points and left there permanently, current readings taken with a voltmeter as
needed without interrupting continuity in the circuit:
Load
V
Rshunt
Of course, care must be taken in sizing the shunt resistor low enough so that it doesn't
adversely affect the circuit's normal operation, but this is generally not difficult to do. This
technique might also be useful in computer circuit analysis, where we might want to have the
computer display current through a circuit in terms of a voltage (with SPICE, this would allow
us to avoid the idiosyncrasy of reading negative current values):
260
CHAPTER 8. DC METERING CIRCUITS
1
2
0
0
Rshunt
1 Ω
Rload
15 kΩ
12 V
shunt resistor example circuit
v1 1 0
rshunt 1 2 1
rload 2 0 15k
.dc v1 12 12 1
.print dc v(1,2)
.end
v1
v(1,2)
1.200E+01
7.999E-04
We would interpret the voltage reading across the shunt resistor (between circuit nodes 1
and 2 in the SPICE simulation) directly as amps, with 7.999E-04 being 0.7999 mA, or 799.9 μA.
Ideally, 12 volts applied directly across 15 kΩwould give us exactly 0.8 mA, but the resistance
of the shunt lessens that current just a tiny bit (as it would in real life). However, such a
tiny error is generally well within acceptable limits of accuracy for either a simulation or a
real circuit, and so shunt resistors can be used in all but the most demanding applications for
accurate current measurement.
• REVIEW:
• Ammeter ranges are created by adding parallel "shunt" resistors to the movement circuit,
providing a precise current division.
• Shunt resistors may have high power dissipations, so be careful when choosing parts for
such meters!
• Shunt resistors can be used in conjunction with high-resistance voltmeters as well as
low-resistance ammeter movements, producing accurate voltage drops for given amounts
of current. Shunt resistors should be selected for as low a resistance value as possible to
minimize their impact upon the circuit under test.
8.5
Ammeter impact on measured circuit
Just like voltmeters, ammeters tend to influence the amount of current in the circuits they're
connected to. However, unlike the ideal voltmeter, the ideal ammeter has zero internal resis-
tance, so as to drop as little voltage as possible as electrons flow through it. Note that this ideal
8.5.
AMMETER IMPACT ON MEASURED CIRCUIT
261
resistance value is exactly opposite as that of a voltmeter. With voltmeters, we want as little
current to be drawn as possible from the circuit under test. With ammeters, we want as little
voltage to be dropped as possible while conducting current.
Here is an extreme example of an ammeter's effect upon a circuit:
A
R1
R2
3 Ω
1.5 Ω
666.7 mA
1.333 A
0.5 Ω
Rinternal
2 V
With the ammeter disconnected from this circuit, the current through the 3 Ωresistor would
be 666.7 mA, and the current through the 1.5 Ωresistor would be 1.33 amps. If the ammeter
had an internal resistance of 1/2 Ω, and it were inserted into one of the branches of this circuit,
though, its resistance would seriously affect the measured branch current:
A
R1
R2
3 Ω
1.5 Ω
Rinternal
0.5 Ω
571.43 mA
1.333 A
2 V
Having effectively increased the left branch resistance from 3 Ωto 3.5 Ω, the ammeter will
read 571.43 mA instead of 666.7 mA. Placing the same ammeter in the right branch would
affect the current to an even greater extent:
262
CHAPTER 8. DC METERING CIRCUITS
A
R1
R2
3 Ω
1.5 Ω
2 V
666.7 mA
1 A
Rinternal
0.5 Ω
Now the right branch current is 1 amp instead of 1.333 amps, due to the increase in resis-
tance created by the addition of the ammeter into the current path.
When using standard ammeters that connect in series with the circuit being measured, it
might not be practical or possible to redesign the meter for a lower input (lead-to-lead) re-
sistance. However, if we were selecting a value of shunt resistor to place in the circuit for a
current measurement based on voltage drop, and we had our choice of a wide range of resis-
tances, it would be best to choose the lowest practical resistance for the application. Any more
resistance than necessary and the shunt may impact the circuit adversely by adding excessive
resistance in the current path.
One ingenious way to reduce the impact that a current-measuring device has on a circuit
is to use the circuit wire as part of the ammeter movement itself. All current-carrying wires
produce a magnetic field, the strength of which is in direct proportion to the strength of the
current. By building an instrument that measures the strength of that magnetic field, a no-
contact ammeter can be produced. Such a meter is able to measure the current through a
conductor without even having to make physical contact with the circuit, much less break
continuity or insert additional resistance.
8.5.
AMMETER IMPACT ON MEASURED CIRCUIT
263
current to be
measured
magnetic field
encircling the
current-carrying
conductor
clamp-on
ammeter
Ammeters of this design are made, and are called "clamp-on" meters because they have
"jaws" which can be opened and then secured around a circuit wire. Clamp-on ammeters make
for quick and safe current measurements, especially on high-power industrial circuits. Because
the circuit under test has had no additional resistance inserted into it by a clamp-on meter,
there is no error induced in taking a current measurement.
current to be
measured
magnetic field
encircling the
current-carrying
conductor
clamp-on
ammeter
The actual movement mechanism of a clamp-on ammeter is much the same as for an iron-
vane instrument, except that there is no internal wire coil to generate the magnetic field. More
modern designs of clamp-on ammeters utilize a small magnetic field detector device called
a Hall-effect sensor to accurately determine field strength.
Some clamp-on meters contain
electronic amplifier circuitry to generate a small voltage proportional to the current in the
264
CHAPTER 8. DC METERING CIRCUITS
wire between the jaws, that small voltage connected to a voltmeter for convenient readout by
a technician. Thus, a clamp-on unit can be an accessory device to a voltmeter, for current
measurement.
A less accurate type of magnetic-field-sensing ammeter than the clamp-on style is shown in
the following photograph:
The operating principle for this ammeter is identical to the clamp-on style of meter: the
circular magnetic field surrounding a current-carrying conductor deflects the meter's needle,
producing an indication on the scale. Note how there are two current scales on this particular
meter: +/- 75 amps and +/- 400 amps. These two measurement scales correspond to the two sets
of notches on the back of the meter. Depending on which set of notches the current-carrying
conductor is laid in, a given strength of magnetic field will have a different amount of effect on
the needle. In effect, the two different positions of the conductor relative to the movement act
as two different range resistors in a direct-connection style of ammeter.
• REVIEW:
• An ideal ammeter has zero resistance.
• A "clamp-on" ammeter measures current through a wire by measuring the strength of
the magnetic field around it rather than by becoming part of the circuit, making it an
ideal ammeter.
• Clamp-on meters make for quick and safe current measurements, because there is no
conductive contact between the meter and the circuit.
8.6
Ohmmeter design
Though mechanical ohmmeter (resistance meter) designs are rarely used today, having largely
been superseded by digital instruments, their operation is nonetheless intriguing and worthy
of study.
The purpose of an ohmmeter, of course, is to measure the resistance placed between its
leads. This resistance reading is indicated through a mechanical meter movement which oper-
ates on electric current. The ohmmeter must then have an internal source of voltage to create
the necessary current to operate the movement, and also have appropriate ranging resistors to
allow just the right amount of current through the movement at any given resistance.
Starting with a simple movement and battery circuit, let's see how it would function as an
ohmmeter:
8.6.
OHMMETER DESIGN
265
black test
lead
lead
red test
500 Ω F.S. = 1 mA
9 V
A simple ohmmeter
When there is infinite resistance (no continuity between test leads), there is zero current
through the meter movement, and the needle points toward the far left of the scale. In this
regard, the ohmmeter indication is "backwards" because maximum indication (infinity) is on
the left of the scale, while voltage and current meters have zero at the left of their scales.
If the test leads of this ohmmeter are directly shorted together (measuring zero Ω), the me-
ter movement will have a maximum amount of current through it, limited only by the battery
voltage and the movement's internal resistance:
black test
lead
lead
red test
500 Ω F.S. = 1 mA
9 V
18 mA
With 9 volts of battery potential and only 500 Ωof movement resistance, our circuit current
will be 18 mA, which is far beyond the full-scale rating of the movement. Such an excess of
current will likely damage the meter.
Not only that, but having such a condition limits the usefulness of the device. If full left-
of-scale on the meter face represents an infinite amount of resistance, then full right-of-scale
should represent zero. Currently, our design "pegs" the meter movement hard to the right when
zero resistance is attached between the leads. We need a way to make it so that the movement
just registers full-scale when the test leads are shorted together.
This is accomplished by
adding a series resistance to the meter's circuit:
266
CHAPTER 8. DC METERING CIRCUITS
black test
lead
lead
red test
500 Ω F.S. = 1 mA
9 V
R
To determine the proper value for R, we calculate the total circuit resistance needed to
limit current to 1 mA (full-scale deflection on the movement) with 9 volts of potential from the
battery, then subtract the movement's internal resistance from that figure:
E
I
9 V
1 mA
Rtotal = 9 kΩ
Now that the right value for R has been calculated, we're still left with a problem of meter
range. On the left side of the scale we have "infinity" and on the right side we have zero.
Besides being "backwards" from the scales of voltmeters and ammeters, this scale is strange
because it goes from nothing to everything, rather than from nothing to a finite value (such as
10 volts, 1 amp, etc.). One might pause to wonder, "what does middle-of-scale represent? What
figure lies exactly between zero and infinity?" Infinity is more than just a very big amount:
it is an incalculable quantity, larger than any definite number ever could be. If half-scale
indication on any other type of meter represents 1/2 of the full-scale range value, then what is
half of infinity on an ohmmeter scale?
The answer to this paradox is a nonlinear scale. Simply put, the scale of an ohmmeter does
not smoothly progress from zero to infinity as the needle sweeps from right to left. Rather,
the scale starts out "expanded" at the right-hand side, with the successive resistance values
growing closer and closer to each other toward the left side of the scale:
8.6.
OHMMETER DESIGN
267
0
300
75
100
150
750
1.5k
15k
An ohmmeter's logarithmic scale
Infinity cannot be approached in a linear (even) fashion, because the scale would never get
there! With a nonlinear scale, the amount of resistance spanned for any given distance on the
scale increases as the scale progresses toward infinity, making infinity an attainable goal.
We still have a question of range for our ohmmeter, though.
What value of resistance
between the test leads will cause exactly 1/2 scale deflection of the needle? If we know that the
movement has a full-scale rating of 1 mA, then 0.5 mA (500 μA) must be the value needed for
half-scale deflection. Following our design with the 9 volt battery as a source we get:
E
I
9 V
Rtotal = 18 kΩ
500 μA
With an internal movement resistance of 500 Ωand a series range resistor of 8.5 kΩ, this
leaves 9 kΩfor an external (lead-to-lead) test resistance at 1/2 scale. In other words, the test
resistance giving 1/2 scale deflection in an ohmmeter is equal in value to the (internal) series
total resistance of the meter circuit.
Using Ohm's Law a few more times, we can determine the test resistance value for 1/4 and
3/4 scale deflection as well:
1/4 scale deflection (0.25 mA of meter current):
268
CHAPTER 8. DC METERING CIRCUITS
E
I
9 V
Rtotal = 36 kΩ
250 μA
Rtest = Rtotal - Rinternal
Rtest = 27 kΩ
3/4 scale deflection (0.75 mA of meter current):
E
I
9 V
Rtest = Rtotal - Rinternal
750 μA
12 kΩ
Rtest = 3 kΩ
So, the scale for this ohmmeter looks something like this:
0
9k
3k
27k
One major problem with this design is its reliance upon a stable battery voltage for accurate
8.7.
HIGH VOLTAGE OHMMETERS
269
resistance reading. If the battery voltage decreases (as all chemical batteries do with age and
use), the ohmmeter scale will lose accuracy. With the series range resistor at a constant value
of 8.5 kΩand the battery voltage decreasing, the meter will no longer deflect full-scale to the
right when the test leads are shorted together (0 Ω). Likewise, a test resistance of 9 kΩwill
fail to deflect the needle to exactly 1/2 scale with a lesser battery voltage.
There are design techniques used to compensate for varying battery voltage, but they do
not completely take care of the problem and are to be considered approximations at best. For
this reason, and for the fact of the nonlinear scale, this type of ohmmeter is never considered
to be a precision instrument.
One final caveat needs to be mentioned with regard to ohmmeters: they only function cor-
rectly when measuring resistance that is not being powered by a voltage or current source. In
other words, you cannot measure resistance with an ohmmeter on a "live" circuit! The reason
for this is simple: the ohmmeter's accurate indication depends on the only source of voltage be-
ing its internal battery. The presence of any voltage across the component to be measured will
interfere with the ohmmeter's operation. If the voltage is large enough, it may even damage
the ohmmeter.
• REVIEW:
• Ohmmeters contain internal sources of voltage to supply power in taking resistance mea-
surements.
• An analog ohmmeter scale is "backwards" from that of a voltmeter or ammeter, the move-
ment needle reading zero resistance at full-scale and infinite resistance at rest.
• Analog ohmmeters also have nonlinear scales, "expanded" at the low end of the scale and
"compressed" at the high end to be able to span from zero to infinite resistance.
• Analog ohmmeters are not precision instruments.
• Ohmmeters should never be connected to an energized circuit (that is, a circuit with its
own source of voltage). Any voltage applied to the test leads of an ohmmeter will invali-
date its reading.
8.7
High voltage ohmmeters
Most ohmmeters of the design shown in the previous section utilize a battery of relatively low
voltage, usually nine volts or less. This is perfectly adequate for measuring resistances under
several mega-ohms (MΩ), but when extremely high resistances need to be measured, a 9 volt
battery is insufficient for generating enough current to actuate an electromechanical meter
movement.
Also, as discussed in an earlier chapter, resistance is not always a stable (linear) quantity.
This is especially true of non-metals. Recall the graph of current over voltage for a small air
gap (less than an inch):
270
CHAPTER 8. DC METERING CIRCUITS
I
(current)
E
(voltage)
ionization potential
0
50
100
150
200
250
300
350
400
While this is an extreme example of nonlinear conduction, other substances exhibit similar
insulating/conducting properties when exposed to high voltages. Obviously, an ohmmeter using
a low-voltage battery as a source of power cannot measure resistance at the ionization potential
of a gas, or at the breakdown voltage of an insulator. If such resistance values need to be
measured, nothing but a high-voltage ohmmeter will suffice.
The most direct method of high-voltage resistance measurement involves simply substitut-
ing a higher voltage battery in the same basic design of ohmmeter investigated earlier:
black test
lead
lead
red test
Simple high-voltage ohmmeter
Knowing, however, that the resistance of some materials tends to change with applied volt-
age, it would be advantageous to be able to adjust the voltage of this ohmmeter to obtain
resistance measurements under different conditions:
8.7.
HIGH VOLTAGE OHMMETERS
271
black test
lead
lead
red test
Unfortunately, this would create a calibration problem for the meter. If the meter movement
deflects full-scale with a certain amount of current through it, the full-scale range of the meter
in ohms would change as the source voltage changed. Imagine connecting a stable resistance
across the test leads of this ohmmeter while varying the source voltage: as the voltage is
increased, there will be more current through the meter movement, hence a greater amount
of deflection. What we really need is a meter movement that will produce a consistent, stable
deflection for any stable resistance value measured, regardless of the applied voltage.
Accomplishing this design goal requires a special meter movement, one that is peculiar to
megohmmeters, or meggers, as these instruments are known.
0
Magnet
Magnet
1
1
2
2
3
3
"Megger" movement
The numbered, rectangular blocks in the above illustration are cross-sectional representa-
tions of wire coils. These three coils all move with the needle mechanism. There is no spring
mechanism to return the needle to a set position. When the movement is unpowered, the
needle will randomly "float." The coils are electrically connected like this:
272
CHAPTER 8. DC METERING CIRCUITS
2
3
1
Test leads
Red
Black
High voltage
With infinite resistance between the test leads (open circuit), there will be no current
through coil 1, only through coils 2 and 3. When energized, these coils try to center them-
selves in the gap between the two magnet poles, driving the needle fully to the right of the
scale where it points to "infinity."
0
Magnet
Magnet
1
1
2
3
Current through coils 2 and 3;
no current through coil 1
Any current through coil 1 (through a measured resistance connected between the test
leads) tends to drive the needle to the left of scale, back to zero. The internal resistor values of
the meter movement are calibrated so that when the test leads are shorted together, the needle
deflects exactly to the 0 Ωposition.
Because any variations in battery voltage will affect the torque generated by both sets of
8.7.
HIGH VOLTAGE OHMMETERS
273
coils (coils 2 and 3, which drive the needle to the right, and coil 1, which drives the needle to the
left), those variations will have no effect of the calibration of the movement. In other words,
the accuracy of this ohmmeter movement is unaffected by battery voltage: a given amount
of measured resistance will produce a certain needle deflection, no matter how much or little
battery voltage is present.
The only effect that a variation in voltage will have on meter indication is the degree to
which the measured resistance changes with applied voltage. So, if we were to use a megger
to measure the resistance of a gas-discharge lamp, it would read very high resistance (needle
to the far right of the scale) for low voltages and low resistance (needle moves to the left of the
scale) for high voltages. This is precisely what we expect from a good high-voltage ohmmeter:
to provide accurate indication of subject resistance under different circumstances.
For maximum safety, most meggers are equipped with hand-crank generators for producing
the high DC voltage (up to 1000 volts). If the operator of the meter receives a shock from the
high voltage, the condition will be self-correcting, as he or she will naturally stop cranking
the generator! Sometimes a "slip clutch" is used to stabilize generator speed under different
cranking conditions, so as to provide a fairly stable voltage whether it is cranked fast or slow.
Multiple voltage output levels from the generator are available by the setting of a selector
switch.
A simple hand-crank megger is shown in this photograph:
Some meggers are battery-powered to provide greater precision in output voltage.
For
safety reasons these meggers are activated by a momentary-contact pushbutton switch, so
the switch cannot be left in the "on" position and pose a significant shock hazard to the meter
operator.
Real meggers are equipped with three connection terminals, labeled Line, Earth, and Guard.
The schematic is quite similar to the simplified version shown earlier:
274
CHAPTER 8. DC METERING CIRCUITS
2
3
1
High voltage
Earth
Line
Guard
Resistance is measured between the Line and Earth terminals, where current will travel
through coil 1. The "Guard" terminal is provided for special testing situations where one re-
sistance must be isolated from another. Take for instance this scenario where the insulation
resistance is to be tested in a two-wire cable:
conductor
insulation
conductor
sheath
cable
Cable
To measure insulation resistance from a conductor to the outside of the cable, we need to
connect the "Line" lead of the megger to one of the conductors and connect the "Earth" lead of
the megger to a wire wrapped around the sheath of the cable:
8.7.
HIGH VOLTAGE OHMMETERS
275
wire wrapped
around
cable sheath
L E
G
In this configuration the megger should read the resistance between one conductor and the
outside sheath. Or will it? If we draw a schematic diagram showing all insulation resistances
as resistor symbols, what we have looks like this:
conductor1
conductor2
sheath
Megger
Earth
Line
Rather than just measure the resistance of the second conductor to the sheath (Rc2−s), what
we'll actually measure is that resistance in parallel with the series combination of conductor-
to-conductor resistance (Rc1−c2) and the first conductor to the sheath (Rc1−s). If we don't care
about this fact, we can proceed with the test as configured.
If we desire to measure only
276
CHAPTER 8. DC METERING CIRCUITS
the resistance between the second conductor and the sheath (Rc2−s), then we need to use the
megger's "Guard" terminal:
wire wrapped
around
cable sheath
L E
G
Megger with "Guard"
connected
Now the circuit schematic looks like this:
conductor1
conductor2
sheath
Megger
Earth
Line
Guard
Connecting the "Guard" terminal to the first conductor places the two conductors at almost
equal potential. With little or no voltage between them, the insulation resistance is nearly
infinite, and thus there will be no current between the two conductors.
Consequently, the
8.8.
MULTIMETERS
277
megger's resistance indication will be based exclusively on the current through the second
conductor's insulation, through the cable sheath, and to the wire wrapped around, not the
current leaking through the first conductor's insulation.
Meggers are field instruments: that is, they are designed to be portable and operated by a
technician on the job site with as much ease as a regular ohmmeter. They are very useful for
checking high-resistance "short" failures between wires caused by wet or degraded insulation.
Because they utilize such high voltages, they are not as affected by stray voltages (voltages
less than 1 volt produced by electrochemical reactions between conductors, or "induced" by
neighboring magnetic fields) as ordinary ohmmeters.
For a more thorough test of wire insulation, another high-voltage ohmmeter commonly
called a hi-pot tester is used. These specialized instruments produce voltages in excess of 1 kV,
and may be used for testing the insulating effectiveness of oil, ceramic insulators, and even the
integrity of other high-voltage instruments. Because they are capable of producing such high
voltages, they must be operated with the utmost care, and only by trained personnel.
It should be noted that hi-pot testers and even meggers (in certain conditions) are capable of
damaging wire insulation if incorrectly used. Once an insulating material has been subjected
to breakdown by the application of an excessive voltage, its ability to electrically insulate will
be compromised. Again, these instruments are to be used only by trained personnel.
8.8
Multimeters
Seeing as how a common meter movement can be made to function as a voltmeter, ammeter, or
ohmmeter simply by connecting it to different external resistor networks, it should make sense
that a multi-purpose meter ("multimeter") could be designed in one unit with the appropriate
switch(es) and resistors.
For general purpose electronics work, the multimeter reigns supreme as the instrument of
choice. No other device is able to do so much with so little an investment in parts and elegant
simplicity of operation. As with most things in the world of electronics, the advent of solid-
state components like transistors has revolutionized the way things are done, and multimeter
design is no exception to this rule. However, in keeping with this chapter's emphasis on analog
("old-fashioned") meter technology, I'll show you a few pre-transistor meters.
The unit shown above is typical of a handheld analog multimeter, with ranges for voltage,
current, and resistance measurement. Note the many scales on the face of the meter movement
for the different ranges and functions selectable by the rotary switch. The wires for connecting
278
CHAPTER 8. DC METERING CIRCUITS
this instrument to a circuit (the "test leads") are plugged into the two copper jacks (socket
holes) at the bottom-center of the meter face marked "- TEST +", black and red.
This multimeter (Barnett brand) takes a slightly different design approach than the previ-
ous unit. Note how the rotary selector switch has fewer positions than the previous meter, but
also how there are many more jacks into which the test leads may be plugged into. Each one
of those jacks is labeled with a number indicating the respective full-scale range of the meter.
Lastly, here is a picture of a digital multimeter. Note that the familiar meter movement has
been replaced by a blank, gray-colored display screen. When powered, numerical digits appear
in that screen area, depicting the amount of voltage, current, or resistance being measured.
This particular brand and model of digital meter has a rotary selector switch and four jacks
into which test leads can be plugged. Two leads - one red and one black - are shown plugged
into the meter.
A close examination of this meter will reveal one "common" jack for the black test lead and
three others for the red test lead. The jack into which the red lead is shown inserted is labeled
for voltage and resistance measurement, while the other two jacks are labeled for current (A,
mA, and μA) measurement. This is a wise design feature of the multimeter, requiring the user
to move a test lead plug from one jack to another in order to switch from the voltage measure-
ment to the current measurement function. It would be hazardous to have the meter set in
current measurement mode while connected across a significant source of voltage because of
the low input resistance, and making it necessary to move a test lead plug rather than just
8.8.
MULTIMETERS
279
flip the selector switch to a different position helps ensure that the meter doesn't get set to
measure current unintentionally.
Note that the selector switch still has different positions for voltage and current measure-
ment, so in order for the user to switch between these two modes of measurement they must
switch the position of the red test lead and move the selector switch to a different position.
Also note that neither the selector switch nor the jacks are labeled with measurement
ranges. In other words, there are no "100 volt" or "10 volt" or "1 volt" ranges (or any equivalent
range steps) on this meter. Rather, this meter is "autoranging," meaning that it automatically
picks the appropriate range for the quantity being measured. Autoranging is a feature only
found on digital meters, but not all digital meters.
No two models of multimeters are designed to operate exactly the same, even if they're man-
ufactured by the same company. In order to fully understand the operation of any multimeter,
the owner's manual must be consulted.
Here is a schematic for a simple analog volt/ammeter:
"Common"
jack
Rshunt
Rmultiplier1
Rmultiplier2
Rmultiplier3
V
V
V
A
Off
A
V
In the switch's three lower (most counter-clockwise) positions, the meter movement is con-
nected to the Common and V jacks through one of three different series range resistors
(Rmultiplier1 through Rmultiplier3), and so acts as a voltmeter. In the fourth position, the me-
ter movement is connected in parallel with the shunt resistor, and so acts as an ammeter for
any current entering the common jack and exiting the A jack. In the last (furthest clockwise)
position, the meter movement is disconnected from either red jack, but short-circuited through
the switch. This short-circuiting creates a dampening effect on the needle, guarding against
mechanical shock damage when the meter is handled and moved.
If an ohmmeter function is desired in this multimeter design, it may be substituted for one
of the three voltage ranges as such:
280
CHAPTER 8. DC METERING CIRCUITS
"Common"
jack
Rshunt
Rmultiplier1
Rmultiplier2
V
V
A
Off
A
Ω
V Ω
RΩ
With all three fundamental functions available, this multimeter may also be known as a
volt-ohm-milliammeter.
Obtaining a reading from an analog multimeter when there is a multitude of ranges and
only one meter movement may seem daunting to the new technician. On an analog multimeter,
the meter movement is marked with several scales, each one useful for at least one range
setting. Here is a close-up photograph of the scale from the Barnett multimeter shown earlier
in this section:
Note that there are three types of scales on this meter face: a green scale for resistance at
the top, a set of black scales for DC voltage and current in the middle, and a set of blue scales
for AC voltage and current at the bottom. Both the DC and AC scales have three sub-scales,
one ranging 0 to 2.5, one ranging 0 to 5, and one ranging 0 to 10. The meter operator must
8.9.
KELVIN (4-WIRE) RESISTANCE MEASUREMENT
281
choose whichever scale best matches the range switch and plug settings in order to properly
interpret the meter's indication.
This particular multimeter has several basic voltage measurement ranges: 2.5 volts, 10
volts, 50 volts, 250 volts, 500 volts, and 1000 volts. With the use of the voltage range extender
unit at the top of the multimeter, voltages up to 5000 volts can be measured. Suppose the
meter operator chose to switch the meter into the "volt" function and plug the red test lead
into the 10 volt jack. To interpret the needle's position, he or she would have to read the scale
ending with the number "10". If they moved the red test plug into the 250 volt jack, however,
they would read the meter indication on the scale ending with "2.5", multiplying the direct
indication by a factor of 100 in order to find what the measured voltage was.
If current is measured with this meter, another jack is chosen for the red plug to be inserted
into and the range is selected via a rotary switch. This close-up photograph shows the switch
set to the 2.5 mA position:
Note how all current ranges are power-of-ten multiples of the three scale ranges shown on
the meter face: 2.5, 5, and 10. In some range settings, such as the 2.5 mA for example, the
meter indication may be read directly on the 0 to 2.5 scale. For other range settings (250 μA,
50 mA, 100 mA, and 500 mA), the meter indication must be read off the appropriate scale
and then multiplied by either 10 or 100 to obtain the real figure. The highest current range
available on this meter is obtained with the rotary switch in the 2.5/10 amp position. The
distinction between 2.5 amps and 10 amps is made by the red test plug position: a special "10
amp" jack next to the regular current-measuring jack provides an alternative plug setting to
select the higher range.
Resistance in ohms, of course, is read by a nonlinear scale at the top of the meter face. It
is "backward," just like all battery-operated analog ohmmeters, with zero at the right-hand
side of the face and infinity at the left-hand side. There is only one jack provided on this
particular multimeter for "ohms," so different resistance-measuring ranges must be selected
by the rotary switch. Notice on the switch how five different "multiplier" settings are provided
for measuring resistance: Rx1, Rx10, Rx100, Rx1000, and Rx10000. Just as you might suspect,
the meter indication is given by multiplying whatever needle position is shown on the meter
face by the power-of-ten multiplying factor set by the rotary switch.
8.9
Kelvin (4-wire) resistance measurement
Suppose we wished to measure the resistance of some component located a significant dis-
tance away from our ohmmeter. Such a scenario would be problematic, because an ohmmeter
282
CHAPTER 8. DC METERING CIRCUITS
measures all resistance in the circuit loop, which includes the resistance of the wires (Rwire)
connecting the ohmmeter to the component being measured (Rsubject):
Ω
Rsubject
Rwire
Rwire
Ohmmeter
Ohmmeter indicates Rwire + Rsubject + Rwire
Usually, wire resistance is very small (only a few ohms per hundreds of feet, depending
primarily on the gauge (size) of the wire), but if the connecting wires are very long, and/or the
component to be measured has a very low resistance anyway, the measurement error intro-
duced by wire resistance will be substantial.
An ingenious method of measuring the subject resistance in a situation like this involves
the use of both an ammeter and a voltmeter. We know from Ohm's Law that resistance is equal
to voltage divided by current (R = E/I). Thus, we should be able to determine the resistance
of the subject component if we measure the current going through it and the voltage dropped
across it:
Rsubject
Rwire
Rwire
A
V
Ammeter
Voltmeter
Rsubject = Voltmeter indication
Ammeter indication
Current is the same at all points in the circuit, because it is a series loop. Because we're
only measuring voltage dropped across the subject resistance (and not the wires' resistances),
though, the calculated resistance is indicative of the subject component's resistance (Rsubject)
alone.
Our goal, though, was to measure this subject resistance from a distance, so our voltmeter
must be located somewhere near the ammeter, connected across the subject resistance by an-
other pair of wires containing resistance:
8.9.
KELVIN (4-WIRE) RESISTANCE MEASUREMENT
283
Rsubject
Rwire
Rwire
A
V
Ammeter
Voltmeter
Rsubject = Voltmeter indication
Ammeter indication
Rwire
Rwire
At first it appears that we have lost any advantage of measuring resistance this way, be-
cause the voltmeter now has to measure voltage through a long pair of (resistive) wires, intro-
ducing stray resistance back into the measuring circuit again. However, upon closer inspection
it is seen that nothing is lost at all, because the voltmeter's wires carry miniscule current.
Thus, those long lengths of wire connecting the voltmeter across the subject resistance will
drop insignificant amounts of voltage, resulting in a voltmeter indication that is very nearly
the same as if it were connected directly across the subject resistance:
Rsubject
Rwire
Rwire
A
V
Ammeter
Voltmeter
Rwire
Rwire
Any voltage dropped across the main current-carrying wires will not be measured by the
voltmeter, and so do not factor into the resistance calculation at all. Measurement accuracy
may be improved even further if the voltmeter's current is kept to a minimum, either by using a
high-quality (low full-scale current) movement and/or a potentiometric (null-balance) system.
This method of measurement which avoids errors caused by wire resistance is called the
Kelvin, or 4-wire method. Special connecting clips called Kelvin clips are made to facilitate
this kind of connection across a subject resistance:
284
CHAPTER 8. DC METERING CIRCUITS
Rsubject
4-wire cable
C
P
C
P
clip
clip
Kelvin clips
In regular, "alligator" style clips, both halves of the jaw are electrically common to each
other, usually joined at the hinge point. In Kelvin clips, the jaw halves are insulated from
each other at the hinge point, only contacting at the tips where they clasp the wire or terminal
of the subject being measured. Thus, current through the "C" ("current") jaw halves does not
go through the "P" ("potential," or voltage) jaw halves, and will not create any error-inducing
voltage drop along their length:
Rsubject
4-wire cable
C
P
C
P
clip
clip
A
V
Rsubject = Voltmeter indication
Ammeter indication
The same principle of using different contact points for current conduction and voltage
measurement is used in precision shunt resistors for measuring large amounts of current. As
discussed previously, shunt resistors function as current measurement devices by dropping
a precise amount of voltage for every amp of current through them, the voltage drop being
measured by a voltmeter. In this sense, a precision shunt resistor "converts" a current value
into a proportional voltage value. Thus, current may be accurately measured by measuring
voltage dropped across the shunt:
8.9.
KELVIN (4-WIRE) RESISTANCE MEASUREMENT
285
V
current to be
measured
measured
current to be
voltmeter
Rshunt
Current measurement using a shunt resistor and voltmeter is particularly well-suited for
applications involving particularly large magnitudes of current.
In such applications, the
shunt resistor's resistance will likely be in the order of milliohms or microohms, so that only a
modest amount of voltage will be dropped at full current. Resistance this low is comparable to
wire connection resistance, which means voltage measured across such a shunt must be done
so in such a way as to avoid detecting voltage dropped across the current-carrying wire con-
nections, lest huge measurement errors be induced. In order that the voltmeter measure only
the voltage dropped by the shunt resistance itself, without any stray voltages originating from
wire or connection resistance, shunts are usually equipped with four connection terminals:
Voltmeter
Shunt
Measured current
Measured current
In metrological (metrology = "the science of measurement") applications, where accuracy is
of paramount importance, highly precise "standard" resistors are also equipped with four ter-
minals: two for carrying the measured current, and two for conveying the resistor's voltage
286
CHAPTER 8. DC METERING CIRCUITS
drop to the voltmeter. This way, the voltmeter only measures voltage dropped across the pre-
cision resistance itself, without any stray voltages dropped across current-carrying wires or
wire-to-terminal connection resistances.
The following photograph shows a precision standard resistor of 1 Ωvalue immersed in a
temperature-controlled oil bath with a few other standard resistors. Note the two large, outer
terminals for current, and the two small connection terminals for voltage:
Here is another, older (pre-World War II) standard resistor of German manufacture. This
unit has a resistance of 0.001 Ω, and again the four terminal connection points can be seen
as black knobs (metal pads underneath each knob for direct metal-to-metal connection with
the wires), two large knobs for securing the current-carrying wires, and two smaller knobs for
securing the voltmeter ("potential") wires:
Appreciation is extended to the Fluke Corporation in Everett, Washington for allowing me
to photograph these expensive and somewhat rare standard resistors in their primary stan-
dards laboratory.
It should be noted that resistance measurement using both an ammeter and a voltmeter
is subject to compound error. Because the accuracy of both instruments factors in to the final
result, the overall measurement accuracy may be worse than either instrument considered
alone. For instance, if the ammeter is accurate to +/- 1% and the voltmeter is also accurate to
+/- 1%, any measurement dependent on the indications of both instruments may be inaccurate
8.9.
KELVIN (4-WIRE) RESISTANCE MEASUREMENT
287
Greater accuracy may be obtained by replacing the ammeter with a standard resistor, used
as a current-measuring shunt. There will still be compound error between the standard resis-
tor and the voltmeter used to measure voltage drop, but this will be less than with a voltmeter
+ ammeter arrangement because typical standard resistor accuracy far exceeds typical amme-
ter accuracy. Using Kelvin clips to make connection with the subject resistance, the circuit
looks something like this:
Rsubject
C
P
C
P
clip
clip
V
Rstandard
All current-carrying wires in the above circuit are shown in "bold," to easily distinguish
them from wires connecting the voltmeter across both resistances (Rsubject and Rstandard). Ide-
ally, a potentiometric voltmeter is used to ensure as little current through the "potential" wires
as possible.
Rlamp
Rcontacts
Rswitch
V
Voltmeter
V
V
Power supply
set for
constant
current
The Kelvin measurement can be a practical tool for finding poor connections or unexpected
resistance in an electrical circuit. Connect a DC power supply to the circuit and adjust the
power supply so that it supplies a constant current to the circuit as shown in the diagram
above (within the circuit's capabilities, of course). With a digital multimeter set to measure
DC voltage, measure the voltage drop across various points in the circuit. If you know the wire
size, you can estimate the voltage drop you should see and compare this to the voltage drop
you measure. This can be a quick and effective method of finding poor connections in wiring
exposed to the elements, such as in the lighting circuits of a trailer. It can also work well for
288
CHAPTER 8. DC METERING CIRCUITS
unpowered AC conductors (make sure the AC power cannot be turned on). For example, you
can measure the voltage drop across a light switch and determine if the wiring connections to
the switch or the switch's contacts are suspect. To be most effective using this technique, you
should also measure the same type of circuits after they are newly made so you have a feel for
the "correct" values. If you use this technique on new circuits and put the results in a log book,
you have valuable information for troubleshooting in the future.
8.10
Bridge circuits
No text on electrical metering could be called complete without a section on bridge circuits.
These ingenious circuits make use of a null-balance meter to compare two voltages, just like
the laboratory balance scale compares two weights and indicates when they're equal. Unlike
the "potentiometer" circuit used to simply measure an unknown voltage, bridge circuits can be
used to measure all kinds of electrical values, not the least of which being resistance.
The standard bridge circuit, often called a Wheatstone bridge, looks something like this:
Ra
Rb
R1
R2
1
2
null
When the voltage between point 1 and the negative side of the battery is equal to the voltage
between point 2 and the negative side of the battery, the null detector will indicate zero and
the bridge is said to be "balanced." The bridge's state of balance is solely dependent on the
ratios of Ra/Rb and R1/R2, and is quite independent of the supply voltage (battery). To measure
resistance with a Wheatstone bridge, an unknown resistance is connected in the place of Ra
or Rb, while the other three resistors are precision devices of known value. Either of the other
three resistors can be replaced or adjusted until the bridge is balanced, and when balance
has been reached the unknown resistor value can be determined from the ratios of the known
resistances.
A requirement for this to be a measurement system is to have a set of variable resistors
available whose resistances are precisely known, to serve as reference standards. For example,
if we connect a bridge circuit to measure an unknown resistance Rx, we will have to know the
exact values of the other three resistors at balance to determine the value of Rx:
8.10.
BRIDGE CIRCUITS
289
Ra
R1
R2
1
2
Rx
Ra
Rx
R1
R2
Bridge circuit is
null
balanced when:
Each of the four resistances in a bridge circuit are referred to as arms. The resistor in series
with the unknown resistance Rx (this would be Ra in the above schematic) is commonly called
the rheostat of the bridge, while the other two resistors are called the ratio arms of the bridge.
Accurate and stable resistance standards, thankfully, are not that difficult to construct. In
fact, they were some of the first electrical "standard" devices made for scientific purposes. Here
is a photograph of an antique resistance standard unit:
This resistance standard shown here is variable in discrete steps: the amount of resistance
between the connection terminals could be varied with the number and pattern of removable
copper plugs inserted into sockets.
Wheatstone bridges are considered a superior means of resistance measurement to the se-
ries battery-movement-resistor meter circuit discussed in the last section. Unlike that circuit,
with all its nonlinearities (nonlinear scale) and associated inaccuracies, the bridge circuit is
linear (the mathematics describing its operation are based on simple ratios and proportions)
and quite accurate.
Given standard resistances of sufficient precision and a null detector device of sufficient
sensitivity, resistance measurement accuracies of at least +/- 0.05% are attainable with a
Wheatstone bridge. It is the preferred method of resistance measurement in calibration labo-
ratories due to its high accuracy.
There are many variations of the basic Wheatstone bridge circuit. Most DC bridges are
used to measure resistance, while bridges powered by alternating current (AC) may be used to
measure different electrical quantities like inductance, capacitance, and frequency.
An interesting variation of the Wheatstone bridge is the Kelvin Double bridge, used for
measuring very low resistances (typically less than 1/10 of an ohm). Its schematic diagram is
as such:
290
CHAPTER 8. DC METERING CIRCUITS
Ra
Rx
null
Ra and Rx are low-value resistances
RM
RN
Rm
Rn
Kelvin Double bridge
The low-value resistors are represented by thick-line symbols, and the wires connecting
them to the voltage source (carrying high current) are likewise drawn thickly in the schematic.
This oddly-configured bridge is perhaps best understood by beginning with a standard Wheat-
stone bridge set up for measuring low resistance, and evolving it step-by-step into its final form
in an effort to overcome certain problems encountered in the standard Wheatstone configura-
tion.
If we were to use a standard Wheatstone bridge to measure low resistance, it would look
something like this:
8.10.
BRIDGE CIRCUITS
291
Ra
Rx
null
RN
RM
When the null detector indicates zero voltage, we know that the bridge is balanced and that
the ratios Ra/Rx and RM/RN are mathematically equal to each other. Knowing the values of
Ra, RM, and RN therefore provides us with the necessary data to solve for Rx . . . almost.
We have a problem, in that the connections and connecting wires between Ra and Rx possess
resistance as well, and this stray resistance may be substantial compared to the low resistances
of Ra and Rx. These stray resistances will drop substantial voltage, given the high current
through them, and thus will affect the null detector's indication and thus the balance of the
bridge:
292
CHAPTER 8. DC METERING CIRCUITS
Ra
Rx
null
RN
RM
ERa
ERx
Ewire
Ewire
Ewire
Ewire
Stray Ewire voltages will corrupt
the accuracy of Rx's measurement
Since we don't want to measure these stray wire and connection resistances, but only mea-
sure Rx, we must find some way to connect the null detector so that it won't be influenced by
voltage dropped across them. If we connect the null detector and RM/RN ratio arms directly
across the ends of Ra and Rx, this gets us closer to a practical solution:
Ra
Rx
null
RN
RM
Ewire
Ewire
Ewire
Ewire
Now, only the two Ewire voltages
are part of the null detector loop
8.10.
BRIDGE CIRCUITS
293
Now the top two Ewire voltage drops are of no effect to the null detector, and do not influence
the accuracy of Rx's resistance measurement. However, the two remaining Ewire voltage drops
will cause problems, as the wire connecting the lower end of Ra with the top end of Rx is now
shunting across those two voltage drops, and will conduct substantial current, introducing
stray voltage drops along its own length as well.
Knowing that the left side of the null detector must connect to the two near ends of Ra
and Rx in order to avoid introducing those Ewire voltage drops into the null detector's loop, and
that any direct wire connecting those ends of Ra and Rx will itself carry substantial current and
create more stray voltage drops, the only way out of this predicament is to make the connecting
path between the lower end of Ra and the upper end of Rx substantially resistive:
Ra
Rx
null
RN
RM
Ewire
Ewire
Ewire
Ewire
We can manage the stray voltage drops between Ra and Rx by sizing the two new resistors
so that their ratio from upper to lower is the same ratio as the two ratio arms on the other side
of the null detector. This is why these resistors were labeled Rm and Rn in the original Kelvin
Double bridge schematic: to signify their proportionality with RM and RN:
294
CHAPTER 8. DC METERING CIRCUITS
Ra
Rx
null
Ra and Rx are low-value resistances
RM
RN
Rm
Rn
Kelvin Double bridge
With ratio Rm/Rn set equal to ratio RM/RN, rheostat arm resistor Ra is adjusted until the
null detector indicates balance, and then we can say that Ra/Rx is equal to RM/RN, or simply
find Rx by the following equation:
RN
RM
The actual balance equation of the Kelvin Double bridge is as follows (Rwire is the resistance
of the thick, connecting wire between the low-resistance standard Ra and the test resistance
Rx):
Rx
Ra
RN
RM
Rwire
Ra (
Rm
RN
RM
Rn
Rm)
So long as the ratio between RM and RN is equal to the ratio between Rm and Rn, the
balance equation is no more complex than that of a regular Wheatstone bridge, with Rx/Ra
equal to RN/RM, because the last term in the equation will be zero, canceling the effects of all
resistances except Rx, Ra, RM, and RN.
In many Kelvin Double bridge circuits, RM=Rm and RN=Rn. However, the lower the re-
sistances of Rm and Rn, the more sensitive the null detector will be, because there is less
resistance in series with it. Increased detector sensitivity is good, because it allows smaller
imbalances to be detected, and thus a finer degree of bridge balance to be attained. Therefore,
some high-precision Kelvin Double bridges use Rm and Rn values as low as 1/100 of their ratio
arm counterparts (RM and RN, respectively). Unfortunately, though, the lower the values of
Rm and Rn, the more current they will carry, which will increase the effect of any junction
resistances present where Rm and Rn connect to the ends of Ra and Rx. As you can see, high
8.11.
WATTMETER DESIGN
295
instrument accuracy demands that all error-producing factors be taken into account, and often
the best that can be achieved is a compromise minimizing two or more different kinds of errors.
• REVIEW:
• Bridge circuits rely on sensitive null-voltage meters to compare two voltages for equality.
• A Wheatstone bridge can be used to measure resistance by comparing the unknown resis-
tor against precision resistors of known value, much like a laboratory scale measures an
unknown weight by comparing it against known standard weights.
• A Kelvin Double bridge is a variant of the Wheatstone bridge used for measuring very low
resistances. Its additional complexity over the basic Wheatstone design is necessary for
avoiding errors otherwise incurred by stray resistances along the current path between
the low-resistance standard and the resistance being measured.
8.11
Wattmeter design
Power in an electric circuit is the product (multiplication) of voltage and current, so any meter
designed to measure power must account for both of these variables.
A special meter movement designed especially for power measurement is called the dy-
namometer movement, and is similar to a D'Arsonval or Weston movement in that a lightweight
coil of wire is attached to the pointer mechanism. However, unlike the D'Arsonval or Weston
movement, another (stationary) coil is used instead of a permanent magnet to provide the
magnetic field for the moving coil to react against. The moving coil is generally energized by
the voltage in the circuit, while the stationary coil is generally energized by the current in the
circuit. A dynamometer movement connected in a circuit looks something like this:
Electrodynamometer movement
Load
The top (horizontal) coil of wire measures load current while the bottom (vertical) coil mea-
sures load voltage. Just like the lightweight moving coils of voltmeter movements, the (moving)
voltage coil of a dynamometer is typically connected in series with a range resistor so that full
load voltage is not applied to it. Likewise, the (stationary) current coil of a dynamometer may
have precision shunt resistors to divide the load current around it. With custom-built dy-
namometer movements, shunt resistors are less likely to be needed because the stationary coil
can be constructed with as heavy of wire as needed without impacting meter response, unlike
the moving coil which must be constructed of lightweight wire for minimum inertia.
296
CHAPTER 8. DC METERING CIRCUITS
Electrodynamometer movement
current
coil
voltage
coil
(stationary)
(moving)
Rshunt
Rmultiplier
• REVIEW:
• Wattmeters are often designed around dynamometer meter movements, which employ
both voltage and current coils to move a needle.
8.12
Creating custom calibration resistances
Often in the course of designing and building electrical meter circuits, it is necessary to have
precise resistances to obtain the desired range(s). More often than not, the resistance values
required cannot be found in any manufactured resistor unit and therefore must be built by
you.
One solution to this dilemma is to make your own resistor out of a length of special high-
resistance wire. Usually, a small "bobbin" is used as a form for the resulting wire coil, and
the coil is wound in such a way as to eliminate any electromagnetic effects: the desired wire
length is folded in half, and the looped wire wound around the bobbin so that current through
the wire winds clockwise around the bobbin for half the wire's length, then counter-clockwise
for the other half.
This is known as a bifilar winding.
Any magnetic fields generated by
the current are thus canceled, and external magnetic fields cannot induce any voltage in the
resistance wire coil:
8.12.
CREATING CUSTOM CALIBRATION RESISTANCES
297
Special
resistance
wire
Bobbin
Completed resistor
Before winding coil
As you might imagine, this can be a labor-intensive process, especially if more than one
resistor must be built! Another, easier solution to the dilemma of a custom resistance is to
connect multiple fixed-value resistors together in series-parallel fashion to obtain the desired
value of resistance. This solution, although potentially time-intensive in choosing the best
resistor values for making the first resistance, can be duplicated much faster for creating mul-
tiple custom resistances of the same value:
R1
R2
R3
R4
Rtotal
A disadvantage of either technique, though, is the fact that both result in a fixed resistance
value. In a perfect world where meter movements never lose magnetic strength of their per-
manent magnets, where temperature and time have no effect on component resistances, and
where wire connections maintain zero resistance forever, fixed-value resistors work quite well
for establishing the ranges of precision instruments. However, in the real world, it is advanta-
geous to have the ability to calibrate, or adjust, the instrument in the future.
It makes sense, then, to use potentiometers (connected as rheostats, usually) as variable
resistances for range resistors. The potentiometer may be mounted inside the instrument case
so that only a service technician has access to change its value, and the shaft may be locked in
place with thread-fastening compound (ordinary nail polish works well for this!) so that it will
not move if subjected to vibration.
However, most potentiometers provide too large a resistance span over their mechanically-
short movement range to allow for precise adjustment. Suppose you desired a resistance of
8.335 kΩ+/- 1 Ω, and wanted to use a 10 kΩpotentiometer (rheostat) to obtain it. A precision
of 1 Ωout of a span of 10 kΩis 1 part in 10,000, or 1/100 of a percent! Even with a 10-turn
298
CHAPTER 8. DC METERING CIRCUITS
potentiometer, it will be very difficult to adjust it to any value this finely. Such a feat would be
nearly impossible using a standard 3/4 turn potentiometer. So how can we get the resistance
value we need and still have room for adjustment?
The solution to this problem is to use a potentiometer as part of a larger resistance network
which will create a limited adjustment range. Observe the following example:
Rtotal
8 kΩ
1 kΩ
8 kΩ to 9 kΩ
adjustable range
Here, the 1 kΩpotentiometer, connected as a rheostat, provides by itself a 1 kΩspan (a
range of 0 Ωto 1 kΩ). Connected in series with an 8 kΩresistor, this offsets the total resistance
by 8,000 Ω, giving an adjustable range of 8 kΩto 9 kΩ. Now, a precision of +/- 1 Ωrepresents
1 part in 1000, or 1/10 of a percent of potentiometer shaft motion. This is ten times better, in
terms of adjustment sensitivity, than what we had using a 10 kΩpotentiometer.
If we desire to make our adjustment capability even more precise - so we can set the resis-
tance at 8.335 kΩwith even greater precision - we may reduce the span of the potentiometer
by connecting a fixed-value resistor in parallel with it:
Rtotal
8 kΩ
1 kΩ
adjustable range
1 kΩ
8 kΩ to 8.5 kΩ
Now, the calibration span of the resistor network is only 500 Ω, from 8 kΩto 8.5 kΩ. This
makes a precision of +/- 1 Ωequal to 1 part in 500, or 0.2 percent. The adjustment is now
half as sensitive as it was before the addition of the parallel resistor, facilitating much easier
calibration to the target value. The adjustment will not be linear, unfortunately (halfway on
the potentiometer's shaft position will not result in 8.25 kΩtotal resistance, but rather 8.333
kΩ). Still, it is an improvement in terms of sensitivity, and it is a practical solution to our
problem of building an adjustable resistance for a precision instrument!
8.13.
CONTRIBUTORS
299
8.13
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
300
CHAPTER 8. DC METERING CIRCUITS
Chapter 9
ELECTRICAL
INSTRUMENTATION SIGNALS
Contents
9.1
Analog and digital signals
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . 301
9.2
Voltage signal systems . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 304
9.3
Current signal systems
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 306
9.4
Tachogenerators
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 309
9.5
Thermocouples
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 310
9.6
pH measurement . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 315
9.7
Strain gauges
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 321
9.8
Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 328
9.1
Analog and digital signals
Instrumentation is a field of study and work centering on measurement and control of physical
processes. These physical processes include pressure, temperature, flow rate, and chemical
consistency. An instrument is a device that measures and/or acts to control any kind of physical
process. Due to the fact that electrical quantities of voltage and current are easy to measure,
manipulate, and transmit over long distances, they are widely used to represent such physical
variables and transmit the information to remote locations.
A signal is any kind of physical quantity that conveys information.
Audible speech is
certainly a kind of signal, as it conveys the thoughts (information) of one person to another
through the physical medium of sound. Hand gestures are signals, too, conveying information
by means of light. This text is another kind of signal, interpreted by your English-trained mind
as information about electric circuits. In this chapter, the word signal will be used primarily
in reference to an electrical quantity of voltage or current that is used to represent or signify
some other physical quantity.
301
302
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
An analog signal is a kind of signal that is continuously variable, as opposed to having
a limited number of steps along its range (called digital). A well-known example of analog
vs. digital is that of clocks: analog being the type with pointers that slowly rotate around a
circular scale, and digital being the type with decimal number displays or a "second-hand" that
jerks rather than smoothly rotates. The analog clock has no physical limit to how finely it can
display the time, as its "hands" move in a smooth, pauseless fashion. The digital clock, on the
other hand, cannot convey any unit of time smaller than what its display will allow for. The
type of clock with a "second-hand" that jerks in 1-second intervals is a digital device with a
minimum resolution of one second.
Both analog and digital signals find application in modern electronics, and the distinctions
between these two basic forms of information is something to be covered in much greater detail
later in this book. For now, I will limit the scope of this discussion to analog signals, since the
systems using them tend to be of simpler design.
With many physical quantities, especially electrical, analog variability is easy to come by.
If such a physical quantity is used as a signal medium, it will be able to represent variations
of information with almost unlimited resolution.
In the early days of industrial instrumentation, compressed air was used as a signaling
medium to convey information from measuring instruments to indicating and controlling de-
vices located remotely. The amount of air pressure corresponded to the magnitude of whatever
variable was being measured. Clean, dry air at approximately 20 pounds per square inch (PSI)
was supplied from an air compressor through tubing to the measuring instrument and was
then regulated by that instrument according to the quantity being measured to produce a cor-
responding output signal. For example, a pneumatic (air signal) level "transmitter" device set
up to measure height of water (the "process variable") in a storage tank would output a low air
pressure when the tank was empty, a medium pressure when the tank was partially full, and
a high pressure when the tank was completely full.
Water
Storage tank
LT
LI
20 PSI compressed
air supply
air flow
analog air pressure
signal
pipe or tube
pipe or tube
water "level transmitter"
(LT)
water "level indicator"
(LI)
The "water level indicator" (LI) is nothing more than a pressure gauge measuring the air
pressure in the pneumatic signal line. This air pressure, being a signal, is in turn a represen-
tation of the water level in the tank. Any variation of level in the tank can be represented by
an appropriate variation in the pressure of the pneumatic signal. Aside from certain practical
9.1.
ANALOG AND DIGITAL SIGNALS
303
limits imposed by the mechanics of air pressure devices, this pneumatic signal is infinitely
variable, able to represent any degree of change in the water's level, and is therefore analog in
the truest sense of the word.
Crude as it may appear, this kind of pneumatic signaling system formed the backbone
of many industrial measurement and control systems around the world, and still sees use
today due to its simplicity, safety, and reliability. Air pressure signals are easily transmitted
through inexpensive tubes, easily measured (with mechanical pressure gauges), and are easily
manipulated by mechanical devices using bellows, diaphragms, valves, and other pneumatic
devices. Air pressure signals are not only useful for measuring physical processes, but for
controlling them as well. With a large enough piston or diaphragm, a small air pressure signal
can be used to generate a large mechanical force, which can be used to move a valve or other
controlling device. Complete automatic control systems have been made using air pressure as
the signal medium. They are simple, reliable, and relatively easy to understand. However, the
practical limits for air pressure signal accuracy can be too limiting in some cases, especially
when the compressed air is not clean and dry, and when the possibility for tubing leaks exist.
With the advent of solid-state electronic amplifiers and other technological advances, elec-
trical quantities of voltage and current became practical for use as analog instrument signaling
media. Instead of using pneumatic pressure signals to relay information about the fullness of
a water storage tank, electrical signals could relay that same information over thin wires (in-
stead of tubing) and not require the support of such expensive equipment as air compressors
to operate:
Water
Storage tank
LT
LI
water "level transmitter"
(LT)
water "level indicator"
(LI)
analog electric
current signal
24 V
Analog electronic signals are still the primary kinds of signals used in the instrumentation
world today (January of 2001), but it is giving way to digital modes of communication in many
applications (more on that subject later). Despite changes in technology, it is always good to
have a thorough understanding of fundamental principles, so the following information will
never really become obsolete.
One important concept applied in many analog instrumentation signal systems is that of
"live zero," a standard way of scaling a signal so that an indication of 0 percent can be discrim-
inated from the status of a "dead" system. Take the pneumatic signal system as an example:
if the signal pressure range for transmitter and indicator was designed to be 0 to 12 PSI, with
0 PSI representing 0 percent of process measurement and 12 PSI representing 100 percent, a
304
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
received signal of 0 percent could be a legitimate reading of 0 percent measurement or it could
mean that the system was malfunctioning (air compressor stopped, tubing broken, transmitter
malfunctioning, etc.). With the 0 percent point represented by 0 PSI, there would be no easy
way to distinguish one from the other.
If, however, we were to scale the instruments (transmitter and indicator) to use a scale of
3 to 15 PSI, with 3 PSI representing 0 percent and 15 PSI representing 100 percent, any kind
of a malfunction resulting in zero air pressure at the indicator would generate a reading of -25
percent (0 PSI), which is clearly a faulty value. The person looking at the indicator would then
be able to immediately tell that something was wrong.
Not all signal standards have been set up with live zero baselines, but the more robust
signals standards (3-15 PSI, 4-20 mA) have, and for good reason.
• REVIEW:
• A signal is any kind of detectable quantity used to communicate information.
• An analog signal is a signal that can be continuously, or infinitely, varied to represent
any small amount of change.
• Pneumatic, or air pressure, signals used to be used predominately in industrial instru-
mentation signal systems. This has been largely superseded by analog electrical signals
such as voltage and current.
• A live zero refers to an analog signal scale using a non-zero quantity to represent 0 percent
of real-world measurement, so that any system malfunction resulting in a natural "rest"
state of zero signal pressure, voltage, or current can be immediately recognized.
9.2
Voltage signal systems
The use of variable voltage for instrumentation signals seems a rather obvious option to ex-
plore. Let's see how a voltage signal instrument might be used to measure and relay informa-
tion about water tank level:
float
potentiometer
moved by float
V
two-conductor cable
Level transmitter
Level indicator
The "transmitter" in this diagram contains its own precision regulated source of voltage,
and the potentiometer setting is varied by the motion of a float inside the water tank following
9.2.
VOLTAGE SIGNAL SYSTEMS
305
the water level. The "indicator" is nothing more than a voltmeter with a scale calibrated to
read in some unit height of water (inches, feet, meters) instead of volts.
As the water tank level changes, the float will move. As the float moves, the potentiometer
wiper will correspondingly be moved, dividing a different proportion of the battery voltage to
go across the two-conductor cable and on to the level indicator. As a result, the voltage received
by the indicator will be representative of the level of water in the storage tank.
This elementary transmitter/indicator system is reliable and easy to understand, but it has
its limitations. Perhaps greatest is the fact that the system accuracy can be influenced by
excessive cable resistance. Remember that real voltmeters draw small amounts of current,
even though it is ideal for a voltmeter not to draw any current at all. This being the case,
especially for the kind of heavy, rugged analog meter movement likely used for an industrial-
quality system, there will be a small amount of current through the 2-conductor cable wires.
The cable, having a small amount of resistance along its length, will consequently drop a small
amount of voltage, leaving less voltage across the indicator's leads than what is across the leads
of the transmitter. This loss of voltage, however small, constitutes an error in measurement:
float
potentiometer
moved by float
V
Level transmitter
Level indicator
voltage drop
Due to voltage drops along
cable conductors, there will be
slightly less voltage across the
output
indicator (meter) than there is
at the output of the transmitter.
voltage drop
Resistor symbols have been added to the wires of the cable to show what is happening in a
real system. Bear in mind that these resistances can be minimized with heavy-gauge wire (at
additional expense) and/or their effects mitigated through the use of a high-resistance (null-
balance?) voltmeter for an indicator (at additional complexity).
Despite this inherent disadvantage, voltage signals are still used in many applications be-
cause of their extreme design simplicity. One common signal standard is 0-10 volts, meaning
that a signal of 0 volts represents 0 percent of measurement, 10 volts represents 100 percent
of measurement, 5 volts represents 50 percent of measurement, and so on. Instruments de-
signed to output and/or accept this standard signal range are available for purchase from major
manufacturers. A more common voltage range is 1-5 volts, which makes use of the "live zero"
concept for circuit fault indication.
• REVIEW:
• DC voltage can be used as an analog signal to relay information from one location to
another.
306
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
• A major disadvantage of voltage signaling is the possibility that the voltage at the indi-
cator (voltmeter) will be less than the voltage at the signal source, due to line resistance
and indicator current draw. This drop in voltage along the conductor length constitutes a
measurement error from transmitter to indicator.
9.3
Current signal systems
It is possible through the use of electronic amplifiers to design a circuit outputting a constant
amount of current rather than a constant amount of voltage. This collection of components is
collectively known as a current source, and its symbol looks like this:
current source
A current source generates as much or as little voltage as needed across its leads to produce
a constant amount of current through it. This is just the opposite of a voltage source (an ideal
battery), which will output as much or as little current as demanded by the external circuit in
maintaining its output voltage constant. Following the "conventional flow" symbology typical
of electronic devices, the arrow points against the direction of electron motion. Apologies for
this confusing notation: another legacy of Benjamin Franklin's false assumption of electron
flow!
current source
electron flow
electron flow
Current in this circuit remains
constant, regardless of circuit
resistance. Only voltage will
change!
Current sources can be built as variable devices, just like voltage sources, and they can
be designed to produce very precise amounts of current. If a transmitter device were to be
constructed with a variable current source instead of a variable voltage source, we could design
an instrumentation signal system based on current instead of voltage:
9.3.
CURRENT SIGNAL SYSTEMS
307
float
Level transmitter
Level indicator
float position changes
output of current source
voltage drop
voltage drop
Being a simple series
circuit, current is equal
at all points, regardless
of any voltage drops!
A
The internal workings of the transmitter's current source need not be a concern at this
point, only the fact that its output varies in response to changes in the float position, just like
the potentiometer setup in the voltage signal system varied voltage output according to float
position.
Notice now how the indicator is an ammeter rather than a voltmeter (the scale calibrated
in inches, feet, or meters of water in the tank, as always). Because the circuit is a series
configuration (accounting for the cable resistances), current will be precisely equal through all
components. With or without cable resistance, the current at the indicator is exactly the same
as the current at the transmitter, and therefore there is no error incurred as there might be
with a voltage signal system. This assurance of zero signal degradation is a decided advantage
of current signal systems over voltage signal systems.
The most common current signal standard in modern use is the 4 to 20 milliamp (4-20 mA)
loop, with 4 milliamps representing 0 percent of measurement, 20 milliamps representing 100
percent, 12 milliamps representing 50 percent, and so on. A convenient feature of the 4-20 mA
standard is its ease of signal conversion to 1-5 volt indicating instruments. A simple 250 ohm
precision resistor connected in series with the circuit will produce 1 volt of drop at 4 milliamps,
5 volts of drop at 20 milliamps, etc:
308
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
A
Transmitter
V
Indicator (1-5 V instrument)
250 Ω
4 - 20 mA current signal
Indicator
(4-20 mA instrument)
| Percent of
signal
signal
0
4.0 mA
1.0 V
10
5.6 mA
1.4 V
20
7.2 mA
1.8 V
25
8.0 mA
2.0 V
30
8.8 mA
2.2 V
40
10.4 mA
2.6 V
50
12.0 mA
3.0 V
60
13.6 mA
3.4 V
70
15.2 mA
3.8 V
75
16.0 mA
4.0 V
80
16.8 mA
4.2 V
90
18.4 mA
4.6 V
100
20.0 mA
5.0 V
9.4.
TACHOGENERATORS
309
The current loop scale of 4-20 milliamps has not always been the standard for current in-
struments: for a while there was also a 10-50 milliamp standard, but that standard has since
been obsoleted. One reason for the eventual supremacy of the 4-20 milliamp loop was safety:
with lower circuit voltages and lower current levels than in 10-50 mA system designs, there
was less chance for personal shock injury and/or the generation of sparks capable of igniting
flammable atmospheres in certain industrial environments.
• REVIEW:
• A current source is a device (usually constructed of several electronic components) that
outputs a constant amount of current through a circuit, much like a voltage source (ideal
battery) outputting a constant amount of voltage to a circuit.
• A current "loop" instrumentation circuit relies on the series circuit principle of current
being equal through all components to insure no signal error due to wiring resistance.
• The most common analog current signal standard in modern use is the "4 to 20 milliamp
current loop."
9.4
Tachogenerators
An electromechanical generator is a device capable of producing electrical power from mechan-
ical energy, usually the turning of a shaft. When not connected to a load resistance, genera-
tors will generate voltage roughly proportional to shaft speed. With precise construction and
design, generators can be built to produce very precise voltages for certain ranges of shaft
speeds, thus making them well-suited as measurement devices for shaft speed in mechanical
equipment. A generator specially designed and constructed for this use is called a tachometer
or tachogenerator. Often, the word "tach" (pronounced "tack") is used rather than the whole
word.
shaft
V
Tachogenerator
voltmeter with
scale calibrated
in RPM (Revolutions
Per Minute)
By measuring the voltage produced by a tachogenerator, you can easily determine the ro-
tational speed of whatever its mechanically attached to. One of the more common voltage
signal ranges used with tachogenerators is 0 to 10 volts. Obviously, since a tachogenerator
cannot produce voltage when its not turning, the zero cannot be "live" in this signal standard.
Tachogenerators can be purchased with different "full-scale" (10 volt) speeds for different ap-
plications. Although a voltage divider could theoretically be used with a tachogenerator to
extend the measurable speed range in the 0-10 volt scale, it is not advisable to significantly
overspeed a precision instrument like this, or its life will be shortened.
310
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
Tachogenerators can also indicate the direction of rotation by the polarity of the output
voltage. When a permanent-magnet style DC generator's rotational direction is reversed, the
polarity of its output voltage will switch. In measurement and control systems where direc-
tional indication is needed, tachogenerators provide an easy way to determine that.
Tachogenerators are frequently used to measure the speeds of electric motors, engines, and
the equipment they power: conveyor belts, machine tools, mixers, fans, etc.
9.5
Thermocouples
An interesting phenomenon applied in the field of instrumentation is the Seebeck effect, which
is the production of a small voltage across the length of a wire due to a difference in temper-
ature along that wire. This effect is most easily observed and applied with a junction of two
dissimilar metals in contact, each metal producing a different Seebeck voltage along its length,
which translates to a voltage between the two (unjoined) wire ends. Most any pair of dissimilar
metals will produce a measurable voltage when their junction is heated, some combinations of
metals producing more voltage per degree of temperature than others:
iron wire
copper wire
junction
small voltage between wires;
more voltage produced as
junction temperature increases.
Seebeck voltage
Seebeck voltage
(heated)
The Seebeck effect is fairly linear; that is, the voltage produced by a heated junction of
two wires is directly proportional to the temperature. This means that the temperature of the
metal wire junction can be determined by measuring the voltage produced. Thus, the Seebeck
effect provides for us an electric method of temperature measurement.
When a pair of dissimilar metals are joined together for the purpose of measuring temper-
ature, the device formed is called a thermocouple. Thermocouples made for instrumentation
use metals of high purity for an accurate temperature/voltage relationship (as linear and as
predictable as possible).
Seebeck voltages are quite small, in the tens of millivolts for most temperature ranges. This
makes them somewhat difficult to measure accurately. Also, the fact that any junction between
dissimilar metals will produce temperature-dependent voltage creates a problem when we try
to connect the thermocouple to a voltmeter, completing a circuit:
iron wire
copper wire
junction
V
copper wire
copper wire
a second iron/copper
junction is formed!
The second iron/copper junction formed by the connection between the thermocouple and
the meter on the top wire will produce a temperature-dependent voltage opposed in polarity
to the voltage produced at the measurement junction. This means that the voltage between
9.5.
THERMOCOUPLES
311
the voltmeter's copper leads will be a function of the difference in temperature between the
two junctions, and not the temperature at the measurement junction alone. Even for ther-
mocouple types where copper is not one of the dissimilar metals, the combination of the two
metals joining the copper leads of the measuring instrument forms a junction equivalent to the
measurement junction:
iron wire
V
copper wire
copper wire
constantan wire
iron/copper
constantan/copper
the equivalent of a single iron/constantan
junction in opposition to the measurement
junction on the left.
measurement
junction
These two junctions in series form
This second junction is called the reference or cold junction, to distinguish it from the junc-
tion at the measuring end, and there is no way to avoid having one in a thermocouple circuit.
In some applications, a differential temperature measurement between two points is required,
and this inherent property of thermocouples can be exploited to make a very simple measure-
ment system.
iron wire
copper wire
V
iron wire
copper wire
junction
junction
However, in most applications the intent is to measure temperature at a single point only,
and in these cases the second junction becomes a liability to function.
Compensation for the voltage generated by the reference junction is typically performed by
a special circuit designed to measure temperature there and produce a corresponding voltage
to counter the reference junction's effects. At this point you may wonder, "If we have to re-
sort to some other form of temperature measurement just to overcome an idiosyncrasy with
thermocouples, then why bother using thermocouples to measure temperature at all? Why
not just use this other form of temperature measurement, whatever it may be, to do the job?"
The answer is this: because the other forms of temperature measurement used for reference
junction compensation are not as robust or versatile as a thermocouple junction, but do the
job of measuring room temperature at the reference junction site quite well. For example, the
thermocouple measurement junction may be inserted into the 1800 degree (F) flue of a foundry
holding furnace, while the reference junction sits a hundred feet away in a metal cabinet at
ambient temperature, having its temperature measured by a device that could never survive
the heat or corrosive atmosphere of the furnace.
The voltage produced by thermocouple junctions is strictly dependent upon temperature.
Any current in a thermocouple circuit is a function of circuit resistance in opposition to this
voltage (I=E/R). In other words, the relationship between temperature and Seebeck voltage is
fixed, while the relationship between temperature and current is variable, depending on the
312
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
total resistance of the circuit. With heavy enough thermocouple conductors, currents upwards
of hundreds of amps can be generated from a single pair of thermocouple junctions!
(I've
actually seen this in a laboratory experiment, using heavy bars of copper and copper/nickel
alloy to form the junctions and the circuit conductors.)
For measurement purposes, the voltmeter used in a thermocouple circuit is designed to have
a very high resistance so as to avoid any error-inducing voltage drops along the thermocouple
wire. The problem of voltage drop along the conductor length is even more severe here than
with the DC voltage signals discussed earlier, because here we only have a few millivolts of
voltage produced by the junction. We simply cannot afford to have even a single millivolt of
drop along the conductor lengths without incurring serious temperature measurement errors.
Ideally, then, current in a thermocouple circuit is zero. Early thermocouple indicating in-
struments made use of null-balance potentiometric voltage measurement circuitry to measure
the junction voltage. The early Leeds & Northrup "Speedomax" line of temperature indica-
tor/recorders were a good example of this technology. More modern instruments use semicon-
ductor amplifier circuits to allow the thermocouple's voltage signal to drive an indication device
with little or no current drawn in the circuit.
Thermocouples, however, can be built from heavy-gauge wire for low resistance, and con-
nected in such a way so as to generate very high currents for purposes other than temperature
measurement. One such purpose is electric power generation. By connecting many thermo-
couples in series, alternating hot/cold temperatures with each junction, a device called a ther-
mopile can be constructed to produce substantial amounts of voltage and current:
iron wire
copper wire
iron wire
copper wire
copper wire
copper wire
copper wire
iron wire
iron wire
iron wire
copper wire
output voltage
"Thermopile"
With the left and right sets of junctions at the same temperature, the voltage at each junc-
tion will be equal and the opposing polarities would cancel to a final voltage of zero. However,
if the left set of junctions were heated and the right set cooled, the voltage at each left junc-
9.5.
THERMOCOUPLES
313
tion would be greater than each right junction, resulting in a total output voltage equal to the
sum of all junction pair differentials. In a thermopile, this is exactly how things are set up.
A source of heat (combustion, strong radioactive substance, solar heat, etc.) is applied to one
set of junctions, while the other set is bonded to a heat sink of some sort (air- or water-cooled).
Interestingly enough, as electrons flow through an external load circuit connected to the ther-
mopile, heat energy is transferred from the hot junctions to the cold junctions, demonstrating
another thermo-electric phenomenon: the so-called Peltier Effect (electric current transferring
heat energy).
Another application for thermocouples is in the measurement of average temperature be-
tween several locations. The easiest way to do this is to connect several thermocouples in
parallel with each other. The millivolt signal produced by each thermocouple will average out
at the parallel junction point. The voltage differences between the junctions drop along the
resistances of the thermocouple wires:
iron wire
junction
V
copper wire
copper wire
constantan wire
junction
constantan wire
iron wire
junction
constantan wire
iron wire
junction
constantan wire
iron wire
reference junctions
Unfortunately, though, the accurate averaging of these Seebeck voltage potentials relies on
each thermocouple's wire resistances being equal. If the thermocouples are located at different
places and their wires join in parallel at a single location, equal wire length will be unlikely.
The thermocouple having the greatest wire length from point of measurement to parallel con-
nection point will tend to have the greatest resistance, and will therefore have the least effect
on the average voltage produced.
To help compensate for this, additional resistance can be added to each of the parallel ther-
mocouple circuit branches to make their respective resistances more equal. Without custom-
sizing resistors for each branch (to make resistances precisely equal between all the thermo-
couples), it is acceptable to simply install resistors with equal values, significantly higher than
the thermocouple wires' resistances so that those wire resistances will have a much smaller
impact on the total branch resistance. These resistors are called swamping resistors, because
their relatively high values overshadow or "swamp" the resistances of the thermocouple wires
themselves:
314
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
iron wire
junction
V
copper wire
copper wire
constantan wire
junction
constantan wire
iron wire
junction
constantan wire
iron wire
junction
constantan wire
iron wire
The meter will register
a more realistic average
of all junction temperatures
with the "swamping"
resistors in place.
Rswamp
Rswamp
Rswamp
Rswamp
Because thermocouple junctions produce such low voltages, it is imperative that wire con-
nections be very clean and tight for accurate and reliable operation. Also, the location of the
reference junction (the place where the dissimilar-metal thermocouple wires join to standard
copper) must be kept close to the measuring instrument, to ensure that the instrument can
accurately compensate for reference junction temperature. Despite these seemingly restrictive
requirements, thermocouples remain one of the most robust and popular methods of industrial
temperature measurement in modern use.
• REVIEW:
• The Seebeck Effect is the production of a voltage between two dissimilar, joined metals
that is proportional to the temperature of that junction.
• In any thermocouple circuit, there are two equivalent junctions formed between dissim-
ilar metals. The junction placed at the site of intended measurement is called the mea-
surement junction, while the other (single or equivalent) junction is called the reference
junction.
• Two thermocouple junctions can be connected in opposition to each other to generate
a voltage signal proportional to differential temperature between the two junctions. A
collection of junctions so connected for the purpose of generating electricity is called a
thermopile.
• When electrons flow through the junctions of a thermopile, heat energy is transferred
from one set of junctions to the other. This is known as the Peltier Effect.
• Multiple thermocouple junctions can be connected in parallel with each other to generate
a voltage signal representing the average temperature between the junctions. "Swamp-
ing" resistors may be connected in series with each thermocouple to help maintain equal-
ity between the junctions, so the resultant voltage will be more representative of a true
average temperature.
• It is imperative that current in a thermocouple circuit be kept as low as possible for good
measurement accuracy. Also, all related wire connections should be clean and tight. Mere
millivolts of drop at any place in the circuit will cause substantial measurement errors.
9.6.
PH MEASUREMENT
315
9.6
pH measurement
A very important measurement in many liquid chemical processes (industrial, pharmaceutical,
manufacturing, food production, etc.) is that of pH: the measurement of hydrogen ion concen-
tration in a liquid solution. A solution with a low pH value is called an "acid," while one with a
high pH is called a "caustic." The common pH scale extends from 0 (strong acid) to 14 (strong
caustic), with 7 in the middle representing pure water (neutral):
7
6
5
4
3
2
1
0
8
9 10
11 12 13 14
Caustic
Acid
Neutral
The pH scale
pH is defined as follows: the lower-case letter "p" in pH stands for the negative common
(base ten) logarithm, while the upper-case letter "H" stands for the element hydrogen. Thus,
pH is a logarithmic measurement of the number of moles of hydrogen ions (H+) per liter of
solution. Incidentally, the "p" prefix is also used with other types of chemical measurements
where a logarithmic scale is desired, pCO2 (Carbon Dioxide) and pO2 (Oxygen) being two such
examples.
The logarithmic pH scale works like this: a solution with 10−12 moles of H+ ions per liter
has a pH of 12; a solution with 10−3 moles of H+ ions per liter has a pH of 3. While very
uncommon, there is such a thing as an acid with a pH measurement below 0 and a caustic
with a pH above 14. Such solutions, understandably, are quite concentrated and extremely
reactive.
While pH can be measured by color changes in certain chemical powders (the "litmus strip"
being a familiar example from high school chemistry classes), continuous process monitoring
and control of pH requires a more sophisticated approach. The most common approach is the
use of a specially-prepared electrode designed to allow hydrogen ions in the solution to migrate
through a selective barrier, producing a measurable potential (voltage) difference proportional
to the solution's pH:
316
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
liquid solution
electrodes
electrodes is proportional
to the pH of the solution
Voltage produced between
The design and operational theory of pH electrodes is a very complex subject, explored only
briefly here. What is important to understand is that these two electrodes generate a voltage
directly proportional to the pH of the solution.
At a pH of 7 (neutral), the electrodes will
produce 0 volts between them. At a low pH (acid) a voltage will be developed of one polarity,
and at a high pH (caustic) a voltage will be developed of the opposite polarity.
An unfortunate design constraint of pH electrodes is that one of them (called the measure-
ment electrode) must be constructed of special glass to create the ion-selective barrier needed
to screen out hydrogen ions from all the other ions floating around in the solution. This glass is
chemically doped with lithium ions, which is what makes it react electrochemically to hydrogen
ions. Of course, glass is not exactly what you would call a "conductor;" rather, it is an extremely
good insulator. This presents a major problem if our intent is to measure voltage between the
two electrodes. The circuit path from one electrode contact, through the glass barrier, through
the solution, to the other electrode, and back through the other electrode's contact, is one of
extremely high resistance.
The other electrode (called the reference electrode) is made from a chemical solution of
neutral (7) pH buffer solution (usually potassium chloride) allowed to exchange ions with the
process solution through a porous separator, forming a relatively low resistance connection to
the test liquid. At first, one might be inclined to ask: why not just dip a metal wire into the
solution to get an electrical connection to the liquid? The reason this will not work is because
metals tend to be highly reactive in ionic solutions and can produce a significant voltage across
the interface of metal-to-liquid contact. The use of a wet chemical interface with the mea-
sured solution is necessary to avoid creating such a voltage, which of course would be falsely
interpreted by any measuring device as being indicative of pH.
Here is an illustration of the measurement electrode's construction. Note the thin, lithium-
doped glass membrane across which the pH voltage is generated:
9.6.
PH MEASUREMENT
317
silver chloride
tip
seal
silver
wire
very thin glass bulb,
chemically "doped" with
lithium ions so as to react
with hydrogen ions outside
the bulb.
bulb filled with
potassium chloride
voltage produced
across thickness of
glass membrane
"buffer" solution
wire connection point
glass body
MEASUREMENT
ELECTRODE
Here is an illustration of the reference electrode's construction. The porous junction shown
at the bottom of the electrode is where the potassium chloride buffer and process liquid inter-
face with each other:
318
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
silver chloride
tip
silver
wire
potassium chloride
"buffer" solution
wire connection point
ELECTRODE
REFERENCE
glass or plastic body
porous junction
filled with
The measurement electrode's purpose is to generate the voltage used to measure the solu-
tion's pH. This voltage appears across the thickness of the glass, placing the silver wire on one
side of the voltage and the liquid solution on the other. The reference electrode's purpose is to
provide the stable, zero-voltage connection to the liquid solution so that a complete circuit can
be made to measure the glass electrode's voltage. While the reference electrode's connection
to the test liquid may only be a few kilo-ohms, the glass electrode's resistance may range from
ten to nine hundred mega-ohms, depending on electrode design! Being that any current in this
circuit must travel through both electrodes' resistances (and the resistance presented by the
test liquid itself), these resistances are in series with each other and therefore add to make an
even greater total.
An ordinary analog or even digital voltmeter has much too low of an internal resistance to
measure voltage in such a high-resistance circuit. The equivalent circuit diagram of a typical
pH probe circuit illustrates the problem:
9.6.
PH MEASUREMENT
319
V
precision voltmeter
Rmeasurement electrode
Rreference electrode
voltage
produced by
electrodes
400 MΩ
3 kΩ
Even a very small circuit current traveling through the high resistances of each component
in the circuit (especially the measurement electrode's glass membrane), will produce relatively
substantial voltage drops across those resistances, seriously reducing the voltage seen by the
meter. Making matters worse is the fact that the voltage differential generated by the mea-
surement electrode is very small, in the millivolt range (ideally 59.16 millivolts per pH unit at
room temperature). The meter used for this task must be very sensitive and have an extremely
high input resistance.
The most common solution to this measurement problem is to use an amplified meter with
an extremely high internal resistance to measure the electrode voltage, so as to draw as little
current through the circuit as possible. With modern semiconductor components, a voltmeter
with an input resistance of up to 1017 Ωcan be built with little difficulty. Another approach,
seldom seen in contemporary use, is to use a potentiometric "null-balance" voltage measure-
ment setup to measure this voltage without drawing any current from the circuit under test.
If a technician desired to check the voltage output between a pair of pH electrodes, this would
probably be the most practical means of doing so using only standard benchtop metering equip-
ment:
V
Rmeasurement electrode
Rreference electrode
voltage
produced by
electrodes
precision
variable
voltage
source
400 MΩ
3 kΩ
null
As usual, the precision voltage supply would be adjusted by the technician until the null
detector registered zero, then the voltmeter connected in parallel with the supply would be
viewed to obtain a voltage reading. With the detector "nulled" (registering exactly zero), there
should be zero current in the pH electrode circuit, and therefore no voltage dropped across the
resistances of either electrode, giving the real electrode voltage at the voltmeter terminals.
Wiring requirements for pH electrodes tend to be even more severe than thermocouple
wiring, demanding very clean connections and short distances of wire (10 yards or less, even
with gold-plated contacts and shielded cable) for accurate and reliable measurement. As with
thermocouples, however, the disadvantages of electrode pH measurement are offset by the
advantages: good accuracy and relative technical simplicity.
Few instrumentation technologies inspire the awe and mystique commanded by pH mea-
320
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
surement, because it is so widely misunderstood and difficult to troubleshoot. Without elabo-
rating on the exact chemistry of pH measurement, a few words of wisdom can be given here
about pH measurement systems:
• All pH electrodes have a finite life, and that lifespan depends greatly on the type and
severity of service. In some applications, a pH electrode life of one month may be consid-
ered long, and in other applications the same electrode(s) may be expected to last for over
a year.
• Because the glass (measurement) electrode is responsible for generating the pH-proportional
voltage, it is the one to be considered suspect if the measurement system fails to generate
sufficient voltage change for a given change in pH (approximately 59 millivolts per pH
unit), or fails to respond quickly enough to a fast change in test liquid pH.
• If a pH measurement system "drifts," creating offset errors, the problem likely lies with
the reference electrode, which is supposed to provide a zero-voltage connection with the
measured solution.
• Because pH measurement is a logarithmic representation of ion concentration, there is
an incredible range of process conditions represented in the seemingly simple 0-14 pH
scale. Also, due to the nonlinear nature of the logarithmic scale, a change of 1 pH at the
top end (say, from 12 to 13 pH) does not represent the same quantity of chemical activity
change as a change of 1 pH at the bottom end (say, from 2 to 3 pH). Control system
engineers and technicians must be aware of this dynamic if there is to be any hope of
controlling process pH at a stable value.
• The following conditions are hazardous to measurement (glass) electrodes: high tem-
peratures, extreme pH levels (either acidic or alkaline), high ionic concentration in the
liquid, abrasion, hydrofluoric acid in the liquid (HF acid dissolves glass!), and any kind
of material coating on the surface of the glass.
• Temperature changes in the measured liquid affect both the response of the measurement
electrode to a given pH level (ideally at 59 mV per pH unit), and the actual pH of the
liquid. Temperature measurement devices can be inserted into the liquid, and the signals
from those devices used to compensate for the effect of temperature on pH measurement,
but this will only compensate for the measurement electrode's mV/pH response, not the
actual pH change of the process liquid!
Advances are still being made in the field of pH measurement, some of which hold great
promise for overcoming traditional limitations of pH electrodes. One such technology uses a
device called a field-effect transistor to electrostatically measure the voltage produced by an
ion-permeable membrane rather than measure the voltage with an actual voltmeter circuit.
While this technology harbors limitations of its own, it is at least a pioneering concept, and
may prove more practical at a later date.
• REVIEW:
• pH is a representation of hydrogen ion activity in a liquid. It is the negative logarithm of
the amount of hydrogen ions (in moles) per liter of liquid. Thus: 10−11 moles of hydrogen
ions in 1 liter of liquid = 11 pH. 10−5.3 moles of hydrogen ions in 1 liter of liquid = 5.3 pH.
9.7.
STRAIN GAUGES
321
• The basic pH scale extends from 0 (strong acid) to 7 (neutral, pure water) to 14 (strong
caustic). Chemical solutions with pH levels below zero and above 14 are possible, but
rare.
• pH can be measured by measuring the voltage produced between two special electrodes
immersed in the liquid solution.
• One electrode, made of a special glass, is called the measurement electrode. It's job it to
generate a small voltage proportional to pH (ideally 59.16 mV per pH unit).
• The other electrode (called the reference electrode) uses a porous junction between the
measured liquid and a stable, neutral pH buffer solution (usually potassium chloride) to
create a zero-voltage electrical connection to the liquid. This provides a point of continuity
for a complete circuit so that the voltage produced across the thickness of the glass in the
measurement electrode can be measured by an external voltmeter.
• The extremely high resistance of the measurement electrode's glass membrane mandates
the use of a voltmeter with extremely high internal resistance, or a null-balance volt-
meter, to measure the voltage.
9.7
Strain gauges
If a strip of conductive metal is stretched, it will become skinnier and longer, both changes
resulting in an increase of electrical resistance end-to-end. Conversely, if a strip of conductive
metal is placed under compressive force (without buckling), it will broaden and shorten. If
these stresses are kept within the elastic limit of the metal strip (so that the strip does not
permanently deform), the strip can be used as a measuring element for physical force, the
amount of applied force inferred from measuring its resistance.
Such a device is called a strain gauge. Strain gauges are frequently used in mechanical
engineering research and development to measure the stresses generated by machinery. Air-
craft component testing is one area of application, tiny strain-gauge strips glued to structural
members, linkages, and any other critical component of an airframe to measure stress. Most
strain gauges are smaller than a postage stamp, and they look something like this:
Bonded strain gauge
Resistance measured
between these points
Compression causes
resistance decrease
Tension causes
resistance increase
Gauge insensitive
to lateral forces
A strain gauge's conductors are very thin: if made of round wire, about 1/1000 inch in
diameter. Alternatively, strain gauge conductors may be thin strips of metallic film deposited
322
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
on a nonconducting substrate material called the carrier. The latter form of strain gauge is
represented in the previous illustration. The name "bonded gauge" is given to strain gauges
that are glued to a larger structure under stress (called the test specimen). The task of bonding
strain gauges to test specimens may appear to be very simple, but it is not. "Gauging" is a
craft in its own right, absolutely essential for obtaining accurate, stable strain measurements.
It is also possible to use an unmounted gauge wire stretched between two mechanical points
to measure tension, but this technique has its limitations.
Typical strain gauge resistances range from 30 Ωto 3 kΩ(unstressed). This resistance may
change only a fraction of a percent for the full force range of the gauge, given the limitations
imposed by the elastic limits of the gauge material and of the test specimen. Forces great
enough to induce greater resistance changes would permanently deform the test specimen
and/or the gauge conductors themselves, thus ruining the gauge as a measurement device.
Thus, in order to use the strain gauge as a practical instrument, we must measure extremely
small changes in resistance with high accuracy.
Such demanding precision calls for a bridge measurement circuit. Unlike the Wheatstone
bridge shown in the last chapter using a null-balance detector and a human operator to main-
tain a state of balance, a strain gauge bridge circuit indicates measured strain by the degree
of imbalance, and uses a precision voltmeter in the center of the bridge to provide an accurate
measurement of that imbalance:
V
R1
R2
R3
Quarter-bridge strain gauge circuit
strain gauge
Typically, the rheostat arm of the bridge (R2 in the diagram) is set at a value equal to the
strain gauge resistance with no force applied. The two ratio arms of the bridge (R1 and R3)
are set equal to each other. Thus, with no force applied to the strain gauge, the bridge will
be symmetrically balanced and the voltmeter will indicate zero volts, representing zero force
on the strain gauge. As the strain gauge is either compressed or tensed, its resistance will
decrease or increase, respectively, thus unbalancing the bridge and producing an indication at
the voltmeter. This arrangement, with a single element of the bridge changing resistance in
response to the measured variable (mechanical force), is known as a quarter-bridge circuit.
As the distance between the strain gauge and the three other resistances in the bridge
9.7.
STRAIN GAUGES
323
circuit may be substantial, wire resistance has a significant impact on the operation of the
circuit. To illustrate the effects of wire resistance, I'll show the same schematic diagram, but
add two resistor symbols in series with the strain gauge to represent the wires:
V
R1
R2
R3
Rwire1
Rwire2
Rgauge
The strain gauge's resistance (Rgauge) is not the only resistance being measured: the wire
resistances Rwire1 and Rwire2, being in series with Rgauge, also contribute to the resistance of
the lower half of the rheostat arm of the bridge, and consequently contribute to the voltmeter's
indication. This, of course, will be falsely interpreted by the meter as physical strain on the
gauge.
While this effect cannot be completely eliminated in this configuration, it can be minimized
with the addition of a third wire, connecting the right side of the voltmeter directly to the upper
wire of the strain gauge:
V
R1
R2
R3
Rwire1
Rwire2
Rgauge
Rwire3
Three-wire, quarter-bridge
strain gauge circuit
Because the third wire carries practically no current (due to the voltmeter's extremely high
internal resistance), its resistance will not drop any substantial amount of voltage. Notice how
324
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
the resistance of the top wire (Rwire1) has been "bypassed" now that the voltmeter connects
directly to the top terminal of the strain gauge, leaving only the lower wire's resistance (Rwire2)
to contribute any stray resistance in series with the gauge. Not a perfect solution, of course,
but twice as good as the last circuit!
There is a way, however, to reduce wire resistance error far beyond the method just de-
scribed, and also help mitigate another kind of measurement error due to temperature. An
unfortunate characteristic of strain gauges is that of resistance change with changes in tem-
perature. This is a property common to all conductors, some more than others. Thus, our
quarter-bridge circuit as shown (either with two or with three wires connecting the gauge to
the bridge) works as a thermometer just as well as it does a strain indicator. If all we want
to do is measure strain, this is not good. We can transcend this problem, however, by using
a "dummy" strain gauge in place of R2, so that both elements of the rheostat arm will change
resistance in the same proportion when temperature changes, thus canceling the effects of
temperature change:
V
R1
R3
strain gauge
(unstressed)
(stressed)
strain gauge
Quarter-bridge strain gauge circuit
with temperature compensation
Resistors R1 and R3 are of equal resistance value, and the strain gauges are identical to
one another. With no applied force, the bridge should be in a perfectly balanced condition
and the voltmeter should register 0 volts. Both gauges are bonded to the same test specimen,
but only one is placed in a position and orientation so as to be exposed to physical strain
(the active gauge). The other gauge is isolated from all mechanical stress, and acts merely as
a temperature compensation device (the "dummy" gauge). If the temperature changes, both
gauge resistances will change by the same percentage, and the bridge's state of balance will
remain unaffected. Only a differential resistance (difference of resistance between the two
strain gauges) produced by physical force on the test specimen can alter the balance of the
bridge.
Wire resistance doesn't impact the accuracy of the circuit as much as before, because the
wires connecting both strain gauges to the bridge are approximately equal length. Therefore,
the upper and lower sections of the bridge's rheostat arm contain approximately the same
9.7.
STRAIN GAUGES
325
amount of stray resistance, and their effects tend to cancel:
V
R1
R3
strain gauge
(unstressed)
(stressed)
strain gauge
Rwire1
Rwire2
Rwire3
Even though there are now two strain gauges in the bridge circuit, only one is responsive
to mechanical strain, and thus we would still refer to this arrangement as a quarter-bridge.
However, if we were to take the upper strain gauge and position it so that it is exposed to the
opposite force as the lower gauge (i.e. when the upper gauge is compressed, the lower gauge
will be stretched, and vice versa), we will have both gauges responding to strain, and the bridge
will be more responsive to applied force. This utilization is known as a half-bridge. Since both
strain gauges will either increase or decrease resistance by the same proportion in response
to changes in temperature, the effects of temperature change remain canceled and the circuit
will suffer minimal temperature-induced measurement error:
V
strain gauge
(stressed)
strain gauge
(stressed)
R1
R3
Half-bridge strain gauge circuit
An example of how a pair of strain gauges may be bonded to a test specimen so as to yield
326
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
this effect is illustrated here:
Test specimen
Strain gauge #1
Strain gauge #2
R
R
Rgauge#1
Rgauge#2
V
Bridge balanced
With no force applied to the test specimen, both strain gauges have equal resistance and
the bridge circuit is balanced. However, when a downward force is applied to the free end of
the specimen, it will bend downward, stretching gauge #1 and compressing gauge #2 at the
same time:
Strain gauge #1
Strain gauge #2
R
R
Rgauge#1
Rgauge#2
V
Bridge unbalanced
FORCE
Test specimen
In applications where such complementary pairs of strain gauges can be bonded to the test
specimen, it may be advantageous to make all four elements of the bridge "active" for even
greater sensitivity. This is called a full-bridge circuit:
9.7.
STRAIN GAUGES
327
V
strain gauge
(stressed)
strain gauge
(stressed)
strain gauge
(stressed)
strain gauge
(stressed)
Full-bridge strain gauge circuit
Both half-bridge and full-bridge configurations grant greater sensitivity over the quarter-
bridge circuit, but often it is not possible to bond complementary pairs of strain gauges to
the test specimen. Thus, the quarter-bridge circuit is frequently used in strain measurement
systems.
When possible, the full-bridge configuration is the best to use. This is true not only because
it is more sensitive than the others, but because it is linear while the others are not. Quarter-
bridge and half-bridge circuits provide an output (imbalance) signal that is only approximately
proportional to applied strain gauge force. Linearity, or proportionality, of these bridge circuits
is best when the amount of resistance change due to applied force is very small compared to the
nominal resistance of the gauge(s). With a full-bridge, however, the output voltage is directly
proportional to applied force, with no approximation (provided that the change in resistance
caused by the applied force is equal for all four strain gauges!).
Unlike the Wheatstone and Kelvin bridges, which provide measurement at a condition of
perfect balance and therefore function irrespective of source voltage, the amount of source (or
"excitation") voltage matters in an unbalanced bridge like this. Therefore, strain gauge bridges
are rated in millivolts of imbalance produced per volt of excitation, per unit measure of force.
A typical example for a strain gauge of the type used for measuring force in industrial envi-
ronments is 15 mV/V at 1000 pounds. That is, at exactly 1000 pounds applied force (either
compressive or tensile), the bridge will be unbalanced by 15 millivolts for every volt of excita-
tion voltage. Again, such a figure is precise if the bridge circuit is full-active (four active strain
gauges, one in each arm of the bridge), but only approximate for half-bridge and quarter-bridge
arrangements.
Strain gauges may be purchased as complete units, with both strain gauge elements and
bridge resistors in one housing, sealed and encapsulated for protection from the elements, and
equipped with mechanical fastening points for attachment to a machine or structure. Such a
package is typically called a load cell.
Like many of the other topics addressed in this chapter, strain gauge systems can become
quite complex, and a full dissertation on strain gauges would be beyond the scope of this book.
328
CHAPTER 9. ELECTRICAL INSTRUMENTATION SIGNALS
• REVIEW:
• A strain gauge is a thin strip of metal designed to measure mechanical load by changing
resistance when stressed (stretched or compressed within its elastic limit).
• Strain gauge resistance changes are typically measured in a bridge circuit, to allow for
precise measurement of the small resistance changes, and to provide compensation for
resistance variations due to temperature.
9.8
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 10
DC NETWORK ANALYSIS
Contents
10.1 What is network analysis?
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . 329
10.2 Branch current method . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 332
10.3 Mesh current method
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 341
10.3.1 Mesh Current, conventional method
. . . . . . . . . . . . . . . . . . . . . 341
10.3.2 Mesh current by inspection
. . . . . . . . . . . . . . . . . . . . . . . . . . 354
10.4 Node voltage method
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 357
10.5 Introduction to network theorems . . . . . . . . . . . . . . . . . . . . . . . . 361
10.6 Millman's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 361
10.7 Superposition Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 364
10.8 Thevenin's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 369
10.9 Norton's Theorem . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 373
10.10Thevenin-Norton equivalencies . . . . . . . . . . . . . . . . . . . . . . . . . . 377
10.11Millman's Theorem revisited . . . . . . . . . . . . . . . . . . . . . . . . . . . . 379
10.12Maximum Power Transfer Theorem . . . . . . . . . . . . . . . . . . . . . . . 381
10.13∆-Y and Y-∆conversions
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 383
10.14Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 389
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 390
10.1
What is network analysis?
Generally speaking, network analysis is any structured technique used to mathematically an-
alyze a circuit (a "network" of interconnected components). Quite often the technician or engi-
neer will encounter circuits containing multiple sources of power or component configurations
which defy simplification by series/parallel analysis techniques. In those cases, he or she will
be forced to use other means. This chapter presents a few techniques useful in analyzing such
complex circuits.
329
330
CHAPTER 10. DC NETWORK ANALYSIS
To illustrate how even a simple circuit can defy analysis by breakdown into series and
parallel portions, take start with this series-parallel circuit:
R1
R3
R2
B1
To analyze the above circuit, one would first find the equivalent of R2 and R3 in parallel,
then add R1 in series to arrive at a total resistance. Then, taking the voltage of battery B1 with
that total circuit resistance, the total current could be calculated through the use of Ohm's
Law (I=E/R), then that current figure used to calculate voltage drops in the circuit. All in all,
a fairly simple procedure.
However, the addition of just one more battery could change all of that:
R1
R3
R2
B1
B2
Resistors R2 and R3 are no longer in parallel with each other, because B2 has been inserted
into R3's branch of the circuit. Upon closer inspection, it appears there are no two resistors
in this circuit directly in series or parallel with each other. This is the crux of our problem:
in series-parallel analysis, we started off by identifying sets of resistors that were directly in
series or parallel with each other, reducing them to single equivalent resistances. If there are
no resistors in a simple series or parallel configuration with each other, then what can we do?
It should be clear that this seemingly simple circuit, with only three resistors, is impossible
to reduce as a combination of simple series and simple parallel sections: it is something differ-
ent altogether. However, this is not the only type of circuit defying series/parallel analysis:
10.1.
WHAT IS NETWORK ANALYSIS?
331
R1
R2
R3
R4
R5
Here we have a bridge circuit, and for the sake of example we will suppose that it is not
balanced (ratio R1/R4 not equal to ratio R2/R5). If it were balanced, there would be zero current
through R3, and it could be approached as a series/parallel combination circuit (R1−−R4 //
R2−−R5). However, any current through R3 makes a series/parallel analysis impossible. R1 is
not in series with R4 because there's another path for electrons to flow through R3. Neither is
R2 in series with R5 for the same reason. Likewise, R1 is not in parallel with R2 because R3 is
separating their bottom leads. Neither is R4 in parallel with R5. Aaarrggghhhh!
Although it might not be apparent at this point, the heart of the problem is the existence
of multiple unknown quantities. At least in a series/parallel combination circuit, there was a
way to find total resistance and total voltage, leaving total current as a single unknown value
to calculate (and then that current was used to satisfy previously unknown variables in the
reduction process until the entire circuit could be analyzed). With these problems, more than
one parameter (variable) is unknown at the most basic level of circuit simplification.
With the two-battery circuit, there is no way to arrive at a value for "total resistance,"
because there are two sources of power to provide voltage and current (we would need two
"total" resistances in order to proceed with any Ohm's Law calculations). With the unbalanced
bridge circuit, there is such a thing as total resistance across the one battery (paving the way
for a calculation of total current), but that total current immediately splits up into unknown
proportions at each end of the bridge, so no further Ohm's Law calculations for voltage (E=IR)
can be carried out.
So what can we do when we're faced with multiple unknowns in a circuit? The answer
is initially found in a mathematical process known as simultaneous equations or systems of
equations, whereby multiple unknown variables are solved by relating them to each other in
multiple equations. In a scenario with only one unknown (such as every Ohm's Law equation
we've dealt with thus far), there only needs to be a single equation to solve for the single
unknown:
332
CHAPTER 10. DC NETWORK ANALYSIS
E
I R
is unknown;
are known
E
and
I
R
. . . or . . .
E
R
is unknown;
and
are known )
I
E
R
. . . or . . .
I
E
R
is unknown;
and
are known )
R
E
I
However, when we're solving for multiple unknown values, we need to have the same num-
ber of equations as we have unknowns in order to reach a solution. There are several methods
of solving simultaneous equations, all rather intimidating and all too complex for explanation
in this chapter. However, many scientific and programmable calculators are able to solve for
simultaneous unknowns, so it is recommended to use such a calculator when first learning how
to analyze these circuits.
This is not as scary as it may seem at first. Trust me!
Later on we'll see that some clever people have found tricks to avoid having to use simulta-
neous equations on these types of circuits. We call these tricks network theorems, and we will
explore a few later in this chapter.
• REVIEW:
• Some circuit configurations ("networks") cannot be solved by reduction according to se-
ries/parallel circuit rules, due to multiple unknown values.
• Mathematical techniques to solve for multiple unknowns (called "simultaneous equa-
tions" or "systems") can be applied to basic Laws of circuits to solve networks.
10.2
Branch current method
The first and most straightforward network analysis technique is called the Branch Current
Method. In this method, we assume directions of currents in a network, then write equations
describing their relationships to each other through Kirchhoff's and Ohm's Laws. Once we
have one equation for every unknown current, we can solve the simultaneous equations and
determine all currents, and therefore all voltage drops in the network.
Let's use this circuit to illustrate the method:
10.2.
BRANCH CURRENT METHOD
333
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
B1
B2
The first step is to choose a node (junction of wires) in the circuit to use as a point of
reference for our unknown currents. I'll choose the node joining the right of R1, the top of R2,
and the left of R3.
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
chosen node
B1
B2
At this node, guess which directions the three wires' currents take, labeling the three cur-
rents as I1, I2, and I3, respectively. Bear in mind that these directions of current are specula-
tive at this point. Fortunately, if it turns out that any of our guesses were wrong, we will know
when we mathematically solve for the currents (any "wrong" current directions will show up
as negative numbers in our solution).
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
I3
B1
B2
Kirchhoff's Current Law (KCL) tells us that the algebraic sum of currents entering and
exiting a node must equal zero, so we can relate these three currents (I1, I2, and I3) to each
other in a single equation. For the sake of convention, I'll denote any current entering the node
as positive in sign, and any current exiting the node as negative in sign:
334
CHAPTER 10. DC NETWORK ANALYSIS
Kirchhoff's Current Law (KCL)
applied to currents at node
The next step is to label all voltage drop polarities across resistors according to the assumed
directions of the currents. Remember that the "upstream" end of a resistor will always be
negative, and the "downstream" end of a resistor positive with respect to each other, since
electrons are negatively charged:
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
I3
B1
B2
The battery polarities, of course, remain as they were according to their symbology (short
end negative, long end positive). It is OK if the polarity of a resistor's voltage drop doesn't
match with the polarity of the nearest battery, so long as the resistor voltage polarity is cor-
rectly based on the assumed direction of current through it. In some cases we may discover
that current will be forced backwards through a battery, causing this very effect. The impor-
tant thing to remember here is to base all your resistor polarities and subsequent calculations
on the directions of current(s) initially assumed. As stated earlier, if your assumption happens
to be incorrect, it will be apparent once the equations have been solved (by means of a negative
solution). The magnitude of the solution, however, will still be correct.
Kirchhoff's Voltage Law (KVL) tells us that the algebraic sum of all voltages in a loop
must equal zero, so we can create more equations with current terms (I1, I2, and I3) for our
simultaneous equations. To obtain a KVL equation, we must tally voltage drops in a loop of the
circuit, as though we were measuring with a real voltmeter. I'll choose to trace the left loop of
this circuit first, starting from the upper-left corner and moving counter-clockwise (the choice
of starting points and directions is arbitrary). The result will look like this:
10.2.
BRANCH CURRENT METHOD
335
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
0 V
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
a positive voltage
336
CHAPTER 10. DC NETWORK ANALYSIS
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
a positive voltage
Having completed our trace of the left loop, we add these voltage indications together for a
sum of zero:
Kirchhoff's Voltage Law (KVL)
applied to voltage drops in left loop
Of course, we don't yet know what the voltage is across R1 or R2, so we can't insert those
values into the equation as numerical figures at this point. However, we do know that all three
voltages must algebraically add to zero, so the equation is true. We can go a step further and
express the unknown voltages as the product of the corresponding unknown currents (I1 and
I2) and their respective resistors, following Ohm's Law (E=IR), as well as eliminate the 0 term:
Ohm's Law: E = IR
. . . Substituting IR for E in the KVL equation . . .
Since we know what the values of all the resistors are in ohms, we can just substitute those
figures into the equation to simplify things a bit:
You might be wondering why we went through all the trouble of manipulating this equation
from its initial form (-28 + ER2 + ER1). After all, the last two terms are still unknown, so what
advantage is there to expressing them in terms of unknown voltages or as unknown currents
(multiplied by resistances)? The purpose in doing this is to get the KVL equation expressed
using the same unknown variables as the KCL equation, for this is a necessary requirement
for any simultaneous equation solution method. To solve for three unknown currents (I1, I2,
and I3), we must have three equations relating these three currents (not voltages!) together.
Applying the same steps to the right loop of the circuit (starting at the chosen node and
moving counter-clockwise), we get another KVL equation:
10.2.
BRANCH CURRENT METHOD
337
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
a negative voltage
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
0 V
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
338
CHAPTER 10. DC NETWORK ANALYSIS
28 V
7 V
R2
R1
R3
V
red
black
Voltmeter indicates:
a negative voltage
Kirchhoff's Voltage Law (KVL)
applied to voltage drops in right loop
Knowing now that the voltage across each resistor can be and should be expressed as the
product of the corresponding current and the (known) resistance of each resistor, we can re-
write the equation as such:
Now we have a mathematical system of three equations (one KCL equation and two KVL
equations) and three unknowns:
Kirchhoff's Current Law
Kirchhoff's Voltage Law
Kirchhoff's Voltage Law
For some methods of solution (especially any method involving a calculator), it is helpful
to express each unknown term in each equation, with any constant value to the right of the
equal sign, and with any "unity" terms expressed with an explicit coefficient of 1. Re-writing
the equations again, we have:
Kirchhoff's Current Law
Kirchhoff's Voltage Law
Kirchhoff's Voltage Law
All three variables represented
in all three equations
Using whatever solution techniques are available to us, we should arrive at a solution for
10.2.
BRANCH CURRENT METHOD
339
the three unknown current values:
Solutions:
So, I1 is 5 amps, I2 is 4 amps, and I3 is a negative 1 amp. But what does "negative" current
mean? In this case, it means that our assumed direction for I3 was opposite of its real direction.
Going back to our original circuit, we can re-draw the current arrow for I3 (and re-draw the
polarity of R3's voltage drop to match):
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
I3
5 A
1 A
4 A
B1
B2
Notice how current is being pushed backwards through battery 2 (electrons flowing "up")
due to the higher voltage of battery 1 (whose current is pointed "down" as it normally would)!
Despite the fact that battery B2's polarity is trying to push electrons down in that branch of the
circuit, electrons are being forced backwards through it due to the superior voltage of battery
B1. Does this mean that the stronger battery will always "win" and the weaker battery always
get current forced through it backwards? No! It actually depends on both the batteries' relative
voltages and the resistor values in the circuit. The only sure way to determine what's going on
is to take the time to mathematically analyze the network.
Now that we know the magnitude of all currents in this circuit, we can calculate voltage
drops across all resistors with Ohm's Law (E=IR):
Let us now analyze this network using SPICE to verify our voltage figures.[2] We could ana-
lyze current as well with SPICE, but since that requires the insertion of extra components into
the circuit, and because we know that if the voltages are all the same and all the resistances
are the same, the currents must all be the same, I'll opt for the less complex analysis. Here's a
re-drawing of our circuit, complete with node numbers for SPICE to reference:
340
CHAPTER 10. DC NETWORK ANALYSIS
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
1
2
3
0
0
0
B1
B2
network analysis example
v1 1 0
v2 3 0 dc 7
r1 1 2 4
r2 2 0 2
r3 2 3 1
.dc v1 28 28 1
.print dc v(1,2) v(2,0) v(2,3)
.end
v1
v(1,2)
v(2)
v(2,3)
2.800E+01
2.000E+01
8.000E+00
1.000E+00
Sure enough, the voltage figures all turn out to be the same: 20 volts across R1 (nodes 1
and 2), 8 volts across R2 (nodes 2 and 0), and 1 volt across R3 (nodes 2 and 3). Take note of the
signs of all these voltage figures: they're all positive values! SPICE bases its polarities on the
order in which nodes are listed, the first node being positive and the second node negative. For
example, a figure of positive (+) 20 volts between nodes 1 and 2 means that node 1 is positive
with respect to node 2. If the figure had come out negative in the SPICE analysis, we would
have known that our actual polarity was "backwards" (node 1 negative with respect to node 2).
Checking the node orders in the SPICE listing, we can see that the polarities all match what
we determined through the Branch Current method of analysis.
• REVIEW:
• Steps to follow for the "Branch Current" method of analysis:
• (1) Choose a node and assume directions of currents.
• (2) Write a KCL equation relating currents at the node.
• (3) Label resistor voltage drop polarities based on assumed currents.
• (4) Write KVL equations for each loop of the circuit, substituting the product IR for E in
each resistor term of the equations.
10.3.
MESH CURRENT METHOD
341
• (5) Solve for unknown branch currents (simultaneous equations).
• (6) If any solution is negative, then the assumed direction of current for that solution is
wrong!
• (7) Solve for voltage drops across all resistors (E=IR).
10.3
Mesh current method
The Mesh Current Method, also known as the Loop Current Method, is quite similar to the
Branch Current method in that it uses simultaneous equations, Kirchhoff's Voltage Law, and
Ohm's Law to determine unknown currents in a network. It differs from the Branch Current
method in that it does not use Kirchhoff's Current Law, and it is usually able to solve a circuit
with less unknown variables and less simultaneous equations, which is especially nice if you're
forced to solve without a calculator.
10.3.1
Mesh Current, conventional method
Let's see how this method works on the same example problem:
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
B1
B2
The first step in the Mesh Current method is to identify "loops" within the circuit encom-
passing all components. In our example circuit, the loop formed by B1, R1, and R2 will be the
first while the loop formed by B2, R2, and R3 will be the second. The strangest part of the
Mesh Current method is envisioning circulating currents in each of the loops. In fact, this
method gets its name from the idea of these currents meshing together between loops like sets
of spinning gears:
R2
R1
R3
I1
I2
B1
B2
342
CHAPTER 10. DC NETWORK ANALYSIS
The choice of each current's direction is entirely arbitrary, just as in the Branch Current
method, but the resulting equations are easier to solve if the currents are going the same
direction through intersecting components (note how currents I1 and I2 are both going "up"
through resistor R2, where they "mesh," or intersect).
If the assumed direction of a mesh
current is wrong, the answer for that current will have a negative value.
The next step is to label all voltage drop polarities across resistors according to the assumed
directions of the mesh currents. Remember that the "upstream" end of a resistor will always
be negative, and the "downstream" end of a resistor positive with respect to each other, since
electrons are negatively charged. The battery polarities, of course, are dictated by their symbol
orientations in the diagram, and may or may not "agree" with the resistor polarities (assumed
current directions):
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
B1
B2
Using Kirchhoff's Voltage Law, we can now step around each of these loops, generating
equations representative of the component voltage drops and polarities. As with the Branch
Current method, we will denote a resistor's voltage drop as the product of the resistance (in
ohms) and its respective mesh current (that quantity being unknown at this point). Where two
currents mesh together, we will write that term in the equation with resistor current being the
sum of the two meshing currents.
Tracing the left loop of the circuit, starting from the upper-left corner and moving counter-
clockwise (the choice of starting points and directions is ultimately irrelevant), counting polar-
ity as if we had a voltmeter in hand, red lead on the point ahead and black lead on the point
behind, we get this equation:
Notice that the middle term of the equation uses the sum of mesh currents I1 and I2 as
the current through resistor R2. This is because mesh currents I1 and I2 are going the same
direction through R2, and thus complement each other. Distributing the coefficient of 2 to the
I1 and I2 terms, and then combining I1 terms in the equation, we can simplify as such:
10.3.
MESH CURRENT METHOD
343
Original form of equation
. . . distributing to terms within parentheses . . .
. . . combining like terms . . .
Simplified form of equation
At this time we have one equation with two unknowns. To be able to solve for two unknown
mesh currents, we must have two equations. If we trace the other loop of the circuit, we can
obtain another KVL equation and have enough data to solve for the two currents. Creature
of habit that I am, I'll start at the upper-left hand corner of the right loop and trace counter-
clockwise:
Simplifying the equation as before, we end up with:
Now, with two equations, we can use one of several methods to mathematically solve for the
unknown currents I1 and I2:
. . . rearranging equations for easier solution . . .
Solutions:
Knowing that these solutions are values for mesh currents, not branch currents, we must
go back to our diagram to see how they fit together to give currents through all components:
344
CHAPTER 10. DC NETWORK ANALYSIS
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
5 A
B1
B2
The solution of -1 amp for I2 means that our initially assumed direction of current was
incorrect. In actuality, I2 is flowing in a counter-clockwise direction at a value of (positive) 1
amp:
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
5 A
1 A
B1
B2
This change of current direction from what was first assumed will alter the polarity of the
voltage drops across R2 and R3 due to current I2. From here, we can say that the current
through R1 is 5 amps, with the voltage drop across R1 being the product of current and resis-
tance (E=IR), 20 volts (positive on the left and negative on the right). Also, we can safely say
that the current through R3 is 1 amp, with a voltage drop of 1 volt (E=IR), positive on the left
and negative on the right. But what is happening at R2?
Mesh current I1 is going "up" through R2, while mesh current I2 is going "down" through R2.
To determine the actual current through R2, we must see how mesh currents I1 and I2 interact
(in this case they're in opposition), and algebraically add them to arrive at a final value. Since
I1 is going "up" at 5 amps, and I2 is going "down" at 1 amp, the real current through R2 must
be a value of 4 amps, going "up:"
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
5 A
1 A
4 A
B1
B2
10.3.
MESH CURRENT METHOD
345
A current of 4 amps through R2's resistance of 2 Ωgives us a voltage drop of 8 volts (E=IR),
positive on the top and negative on the bottom.
The primary advantage of Mesh Current analysis is that it generally allows for the solution
of a large network with fewer unknown values and fewer simultaneous equations. Our example
problem took three equations to solve the Branch Current method and only two equations using
the Mesh Current method. This advantage is much greater as networks increase in complexity:
R1
R2
R3
R4
R5
B1
B2
To solve this network using Branch Currents, we'd have to establish five variables to ac-
count for each and every unique current in the circuit (I1 through I5). This would require
five equations for solution, in the form of two KCL equations and three KVL equations (two
equations for KCL at the nodes, and three equations for KVL in each loop):
R1
R2
R3
R4
R5
node 1
node 2
I1
I2
I3
I4
I5
B1
B2
Kirchhoff's Current Law at node 1
Kirchhoff's Current Law at node 2
Kirchhoff's Voltage Law in left loop
Kirchhoff's Voltage Law in middle loop
Kirchhoff's Voltage Law in right loop
I suppose if you have nothing better to do with your time than to solve for five unknown
variables with five equations, you might not mind using the Branch Current method of analysis
for this circuit. For those of us who have better things to do with our time, the Mesh Current
method is a whole lot easier, requiring only three unknowns and three equations to solve:
346
CHAPTER 10. DC NETWORK ANALYSIS
R1
R2
R3
R4
R5
I1
I2
I3
B1
B2
Kirchhoff's Voltage Law
Kirchhoff's Voltage Law
Kirchhoff's Voltage Law
in left loop
in middle loop
in right loop
Less equations to work with is a decided advantage, especially when performing simulta-
neous equation solution by hand (without a calculator).
Another type of circuit that lends itself well to Mesh Current is the unbalanced Wheatstone
Bridge. Take this circuit, for example:
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
Since the ratios of R1/R4 and R2/R5 are unequal, we know that there will be voltage across
resistor R3, and some amount of current through it. As discussed at the beginning of this
chapter, this type of circuit is irreducible by normal series-parallel analysis, and may only be
analyzed by some other method.
We could apply the Branch Current method to this circuit, but it would require six currents
(I1 through I6), leading to a very large set of simultaneous equations to solve. Using the Mesh
Current method, though, we may solve for all currents and voltages with much fewer variables.
The first step in the Mesh Current method is to draw just enough mesh currents to account
for all components in the circuit. Looking at our bridge circuit, it should be obvious where to
10.3.
MESH CURRENT METHOD
347
place two of these currents:
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I1
I2
The directions of these mesh currents, of course, is arbitrary. However, two mesh currents
is not enough in this circuit, because neither I1 nor I2 goes through the battery. So, we must
add a third mesh current, I3:
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I1
I2
I3
Here, I have chosen I3 to loop from the bottom side of the battery, through R4, through R1,
and back to the top side of the battery. This is not the only path I could have chosen for I3, but
it seems the simplest.
Now, we must label the resistor voltage drop polarities, following each of the assumed cur-
rents' directions:
348
CHAPTER 10. DC NETWORK ANALYSIS
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I1
I2
I3
Notice something very important here: at resistor R4, the polarities for the respective mesh
currents do not agree. This is because those mesh currents (I2 and I3) are going through R4 in
different directions. This does not preclude the use of the Mesh Current method of analysis,
but it does complicate it a bit. Though later, we will show how to avoid the R4 current clash.
(See Example below)
Generating a KVL equation for the top loop of the bridge, starting from the top node and
tracing in a clockwise direction:
Original form of equation
. . . distributing to terms within parentheses . . .
. . . combining like terms . . .
Simplified form of equation
In this equation, we represent the common directions of currents by their sums through
common resistors. For example, resistor R3, with a value of 100 Ω, has its voltage drop rep-
resented in the above KVL equation by the expression 100(I1 + I2), since both currents I1 and
I2 go through R3 from right to left. The same may be said for resistor R1, with its voltage
drop expression shown as 150(I1 + I3), since both I1 and I3 go from bottom to top through that
resistor, and thus work together to generate its voltage drop.
Generating a KVL equation for the bottom loop of the bridge will not be so easy, since we
have two currents going against each other through resistor R4. Here is how I do it (starting
at the right-hand node, and tracing counter-clockwise):
10.3.
MESH CURRENT METHOD
349
Original form of equation
. . . distributing to terms within parentheses . . .
. . . combining like terms . . .
Simplified form of equation
Note how the second term in the equation's original form has resistor R4's value of 300 Ω
multiplied by the difference between I2 and I3 (I2 - I3). This is how we represent the combined
effect of two mesh currents going in opposite directions through the same component. Choosing
the appropriate mathematical signs is very important here: 300(I2 - I3) does not mean the
same thing as 300(I3 - I2). I chose to write 300(I2 - I3) because I was thinking first of I2's effect
(creating a positive voltage drop, measuring with an imaginary voltmeter across R4, red lead
on the bottom and black lead on the top), and secondarily of I3's effect (creating a negative
voltage drop, red lead on the bottom and black lead on the top). If I had thought in terms of
I3's effect first and I2's effect secondarily, holding my imaginary voltmeter leads in the same
positions (red on bottom and black on top), the expression would have been -300(I3 - I2). Note
that this expression is mathematically equivalent to the first one: +300(I2 - I3).
Well, that takes care of two equations, but I still need a third equation to complete my
simultaneous equation set of three variables, three equations. This third equation must also
include the battery's voltage, which up to this point does not appear in either two of the pre-
vious KVL equations. To generate this equation, I will trace a loop again with my imaginary
voltmeter starting from the battery's bottom (negative) terminal, stepping clockwise (again,
the direction in which I step is arbitrary, and does not need to be the same as the direction of
the mesh current in that loop):
Original form of equation
. . . distributing to terms within parentheses . . .
. . . combining like terms . . .
Simplified form of equation
Solving for I1, I2, and I3 using whatever simultaneous equation method we prefer:
350
CHAPTER 10. DC NETWORK ANALYSIS
Solutions:
I2 = 77.241 mA
I3 = 136.092 mA
Example:
Use Octave to find the solution for I1, I2, and I3 from the above simplified form of equations.
Solution:
In Octave, an open source Matlab R⃝clone, enter the coefficients into the A matrix between
square brackets with column elements comma separated, and rows semicolon separated.[4]
Enter the voltages into the column vector: b. The unknown currents: I1, I2, and I3 are calcu-
lated by the command: x=A\b. These are contained within the x column vector.
300
100
150
100
650
0
0
-0.093793
0.077241
0.136092
The negative value arrived at for I1 tells us that the assumed direction for that mesh current
was incorrect. Thus, the actual current values through each resistor is as such:
10.3.
MESH CURRENT METHOD
351
I1
I2
I3
IR1
IR2
IR3
IR4
IR5
Calculating voltage drops across each resistor:
IR1
IR2
IR3
IR4
IR5
50 Ω
150 Ω
100 Ω
24 V
300 Ω
250 Ω
ER1 = IR1R1 = (42.299 mA)(150 Ω) = 6.3448 V
ER2 = IR2R2 = (93.793 mA)(50 Ω) = 4.6897 V
ER3 = IR3R3 = (16.552 mA)(100 Ω) = 1.6552 V
ER4 = IR4R4 = (58.851 mA)(300 Ω) = 17.6552 V
ER5 = IR5R5 = (77.241 mA)(250 Ω) = 19.3103 V
A SPICE simulation confirms the accuracy of our voltage calculations:[2]
352
CHAPTER 10. DC NETWORK ANALYSIS
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
1
1
0
0
2
3
unbalanced wheatstone bridge
v1 1 0
r1 1 2 150
r2 1 3 50
r3 2 3 100
r4 2 0 300
r5 3 0 250
.dc v1 24 24 1
.print dc v(1,2) v(1,3) v(3,2) v(2,0) v(3,0)
.end
v1
v(1,2)
v(1,3)
v(3,2)
v(2)
v(3)
2.400E+01
6.345E+00
4.690E+00
1.655E+00
1.766E+01
1.931E+01
Example:
(a) Find a new path for current I3 that does not produce a conflicting polarity on any resistor
compared to I1 or I2. R4 was the offending component. (b) Find values for I1, I2, and I3. (c)
Find the five resistor currents and compare to the previous values.
(a) Route I3 through R5, R3 and R1 as shown:
10.3.
MESH CURRENT METHOD
353
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I1
I2
I3
Original form of equations
Simplified form of equations
Note that the conflicting polarity on R4 has been removed. Moreover, none of the other
resistors have conflicting polarities.
(b) Octave, an open source (free) matlab clone, yields a mesh current vector at "x":[4]
300
100
250
100
650
350
0
0
-0.093793
-0.058851
0.136092
Not all currents I1, I2, and I3 are the same (I2) as the previous bridge because of different
loop paths However, the resistor currents compare to the previous values:
ma
+ 136.092 ma = 77.241 ma
Since the resistor currents are the same as the previous values, the resistor voltages will
be identical and need not be calculated again.
• REVIEW:
• Steps to follow for the "Mesh Current" method of analysis:
354
CHAPTER 10. DC NETWORK ANALYSIS
• (1) Draw mesh currents in loops of circuit, enough to account for all components.
• (2) Label resistor voltage drop polarities based on assumed directions of mesh currents.
• (3) Write KVL equations for each loop of the circuit, substituting the product IR for E
in each resistor term of the equation.
Where two mesh currents intersect through a
component, express the current as the algebraic sum of those two mesh currents (i.e. I1
+ I2) if the currents go in the same direction through that component. If not, express the
current as the difference (i.e. I1 - I2).
• (4) Solve for unknown mesh currents (simultaneous equations).
• (5) If any solution is negative, then the assumed current direction is wrong!
• (6) Algebraically add mesh currents to find current in components sharing multiple mesh
currents.
• (7) Solve for voltage drops across all resistors (E=IR).
10.3.2
Mesh current by inspection
We take a second look at the "mesh current method" with all the currents running counter-
clockwise (ccw). The motivation is to simplify the writing of mesh equations by ignoring the
resistor voltage drop polarity. Though, we must pay attention to the polarity of voltage sources
with respect to assumed current direction. The sign of the resistor voltage drops will follow a
fixed pattern.
If we write a set of conventional mesh current equations for the circuit below, where we
do pay attention to the signs of the voltage drop across the resistors, we may rearrange the
coefficients into a fixed pattern:
2 Ω
R2
R1
R3
I1
I2
B1
B2
Mesh equations
Simplified
Once rearranged, we may write equations by inspection. The signs of the coefficients follow
a fixed pattern in the pair above, or the set of three in the rules below.
• Mesh current rules:
• This method assumes electron flow (not conventional current flow) voltage sources. Re-
place any current source in parallel with a resistor with an equivalent voltage source in
series with an equivalent resistance.
• Ignoring current direction or voltage polarity on resistors, draw counterclockwise current
loops traversing all components. Avoid nested loops.
10.3.
MESH CURRENT METHOD
355
• Write voltage-law equations in terms of unknown currents currents: I1, I2, and I3. Equa-
tion 1 coefficient 1, equation 2, coefficient 2, and equation 3 coefficient 3 are the positive
sums of resistors around the respective loops.
• All other coefficients are negative, representative of the resistance common to a pair of
loops. Equation 1 coefficient 2 is the resistor common to loops 1 and 2, coefficient 3 the
resistor common to loops 1 an 3. Repeat for other equations and coefficients.
+(sum of R's loop 1)I1 - (common R loop 1-2)I2 - (common R loop 1-3)I3
-(common R loop 1-2)I1 + (sum of R's loop 2)I2 - (common R loop 2-3)I3
-(common R loop 1-3)I1 - (common R loop 2-3)I2 + (sum of R's loop 3)I3
• The right hand side of the equations is equal to any electron current flow voltage source.
A voltage rise with respect to the counterclockwise assumed current is positive, and 0 for
no voltage source.
• Solve equations for mesh currents:I1, I2, and I3 . Solve for currents through individual
resistors with KCL. Solve for voltages with Ohms Law and KVL.
While the above rules are specific for a three mesh circuit, the rules may be extended to
smaller or larger meshes. The figure below illustrates the application of the rules. The three
currents are all drawn in the same direction, counterclockwise. One KVL equation is written
for each of the three loops.
Note that there is no polarity drawn on the resistors.
We do
not need it to determine the signs of the coefficients. Though we do need to pay attention to
the polarity of the voltage source with respect to current direction. The I3counterclockwise
current traverses the 24V source from (+) to (-). This is a voltage rise for electron current flow.
Therefore, the third equation right hand side is +24V.
R1
R2
R3
R4
R5
24 V
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I1
I2
I3
In Octave, enter the coefficients into the A matrix with column elements comma separated,
and rows semicolon separated. Enter the voltages into the column vector b. Solve for the
unknown currents: I1, I2, and I3 with the command: x=A\b. These currents are contained
within the x column vector. The positive values indicate that the three mesh currents all flow
in the assumed counterclockwise direction.
356
CHAPTER 10. DC NETWORK ANALYSIS
300
650
450
0
0
24
0.093793
0.077241
0.136092
The mesh currents match the previous solution by a different mesh current method.. The
calculation of resistor voltages and currents will be identical to the previous solution. No need
to repeat here.
Note that electrical engineering texts are based on conventional current flow. The loop-
current, mesh-current method in those text will run the assumed mesh currents clockwise.[1]
The conventional current flows out the (+) terminal of the battery through the circuit, returning
to the (-) terminal. A conventional current voltage rise corresponds to tracing the assumed
current from (-) to (+) through any voltage sources.
One more example of a previous circuit follows. The resistance around loop 1 is 6 Ω, around
loop 2: 3 Ω. The resistance common to both loops is 2 Ω. Note the coefficients of I1 and I2 in
the pair of equations. Tracing the assumed counterclockwise loop 1 current through B1 from
(+) to (-) corresponds to an electron current flow voltage rise. Thus, the sign of the 28 V is
positive. The loop 2 counter clockwise assumed current traces (-) to (+) through B2, a voltage
drop. Thus, the sign of B2 is negative, -7 in the 2nd mesh equation. Once again, there are no
polarity markings on the resistors. Nor do they figure into the equations.
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
I1
I2
B1
B2
Mesh equations
The currents I1 = 5 A, and I2 = 1 A are both positive. They both flow in the direction of the
counterclockwise loops. This compares with previous results.
• Summary:
• The modified mesh-current method avoids having to determine the signs of the equation
coefficients by drawing all mesh currents counterclockwise for electron current flow.
• However, we do need to determine the sign of any voltage sources in the loop. The voltage
source is positive if the assumed ccw current flows with the battery (source). The sign is
negative if the assumed ccw current flows against the battery.
10.4.
NODE VOLTAGE METHOD
357
• See rules above for details.
10.4
Node voltage method
The node voltage method of analysis solves for unknown voltages at circuit nodes in terms of
a system of KCL equations. This analysis looks strange because it involves replacing voltage
sources with equivalent current sources. Also, resistor values in ohms are replaced by equiv-
alent conductances in siemens, G = 1/R. The siemens (S) is the unit of conductance, having
replaced the mho unit. In any event S = Ω−1. And S = mho (obsolete).
We start with a circuit having conventional voltage sources. A common node E0 is chosen
as a reference point. The node voltages E1 and E2 are calculated with respect to this point.
R1
R2
R3
R4
R5
B1
B2
4Ω
2Ω
1Ω
5Ω
2.5Ω
10V
−4V
E1
E2
E0
11
1
2
22
0
A voltage source in series with a resistance must be replaced by an equivalent current
source in parallel with the resistance. We will write KCL equations for each node. The right
hand side of the equation is the value of the current source feeding the node.
B1
10V
I1
5A
2Ω
R1
2Ω
R1
(a)
(b)
Replacing voltage sources and associated series resistors with equivalent current sources
and parallel resistors yields the modified circuit. Substitute resistor conductances in siemens
for resistance in ohms.
358
CHAPTER 10. DC NETWORK ANALYSIS
G1
G2
G3
G4
G5
I1
I2
5A
4Α
E1
E2
0.5S
0.4S
1S
0.2S
0.25S
E0
GA
GB
The Parallel conductances (resistors) may be combined by addition of the conductances.
Though, we will not redraw the circuit. The circuit is ready for application of the node voltage
method.
Deriving a general node voltage method, we write a pair of KCL equations in terms of
unknown node voltages V1 and V2 this one time. We do this to illustrate a pattern for writing
equations by inspection.
(1)
(2)
(1)
(2)
The coefficients of the last pair of equations above have been rearranged to show a pattern.
The sum of conductances connected to the first node is the positive coefficient of the first voltage
in equation (1). The sum of conductances connected to the second node is the positive coeffi-
cient of the second voltage in equation (2). The other coefficients are negative, representing
conductances between nodes. For both equations, the right hand side is equal to the respective
current source connected to the node. This pattern allows us to quickly write the equations by
inspection. This leads to a set of rules for the node voltage method of analysis.
• Node voltage rules:
• Convert voltage sources in series with a resistor to an equivalent current source with the
resistor in parallel.
• Change resistor values to conductances.
• Select a reference node(E0)
• Assign unknown voltages (E1)(E2) ... (EN)to remaining nodes.
• Write a KCL equation for each node 1,2, ... N. The positive coefficient of the first voltage
in the first equation is the sum of conductances connected to the node. The coefficient for
the second voltage in the second equation is the sum of conductances connected to that
node. Repeat for coefficient of third voltage, third equation, and other equations. These
coefficients fall on a diagonal.
10.4.
NODE VOLTAGE METHOD
359
• All other coefficients for all equations are negative, representing conductances between
nodes. The first equation, second coefficient is the conductance from node 1 to node 2, the
third coefficient is the conductance from node 1 to node 3. Fill in negative coefficients for
other equations.
• The right hand side of the equations is the current source connected to the respective
nodes.
• Solve system of equations for unknown node voltages.
Example: Set up the equations and solve for the node voltages using the numerical values
in the above figure.
Solution:
5
5
3.8095
The solution of two equations can be performed with a calculator, or with octave (not
shown).[4] The solution is verified with SPICE based on the original schematic diagram with
voltage sources. [2] Though, the circuit with the current sources could have been simulated.
V1 11 0 DC 10
V2 22 0 DC -4
r1 11 1 2
r2 1 0 4
r3 1 2 2.5
r4 2 0 5
r5 2 22 1
.DC V1 10 10 1 V2 -4 -4 1
.print DC V(1) V(2)
.end
v(1)
v(2)
3.809524e+00
One more example. This one has three nodes. We do not list the conductances on the
schematic diagram. However, G1 = 1/R1, etc.
360
CHAPTER 10. DC NETWORK ANALYSIS
R1
R2
R3
R4
R5
100 Ω
300 Ω
250 Ω
150 Ω
50 Ω
I=0.136092
E1
E2
E3
E0
There are three nodes to write equations for by inspection. Note that the coefficients are
positive for equation (1) E1, equation (2) E2, and equation (3) E3. These are the sums of all
conductances connected to the nodes. All other coefficients are negative, representing a con-
ductance between nodes. The right hand side of the equations is the associated current source,
0.136092 A for the only current source at node 1. The other equations are zero on the right
hand side for lack of current sources. We are too lazy to calculate the conductances for the
resistors on the diagram. Thus, the subscripted G's are the coefficients.
= 0.136092
We are so lazy that we enter reciprocal resistances and sums of reciprocal resistances into
the octave "A" matrix, letting octave compute the matrix of conductances after "A=".[4] The
initial entry line was so long that it was split into three rows. This is different than previous
examples. The entered "A" matrix is delineated by starting and ending square brackets. Col-
umn elements are space separated. Rows are "new line" separated. Commas and semicolons
are not need as separators. Though, the current vector at "b" is semicolon separated to yield a
column vector of currents.
0.0266667
-0.0066667
-0.0200000
-0.0066667
0.0200000
-0.0100000
-0.0200000
-0.0100000
0.0340000
0.13609
0.00000
0.00000
10.5.
INTRODUCTION TO NETWORK THEOREMS
361
24.000
17.655
19.310
Note that the "A" matrix diagonal coefficients are positive, That all other coefficients are
negative.
The solution as a voltage vector is at "x". E1 = 24.000 V, E2 = 17.655 V, E3 = 19.310 V. These
three voltages compare to the previous mesh current and SPICE solutions to the unbalanced
bridge problem. This is no coincidence, for the 0.13609 A current source was purposely chosen
to yield the 24 V used as a voltage source in that problem.
• Summary
• Given a network of conductances and current sources, the node voltage method of circuit
analysis solves for unknown node voltages from KCL equations.
• See rules above for details in writing the equations by inspection.
• The unit of conductance G is the siemens S. Conductance is the reciprocal of resistance:
10.5
Introduction to network theorems
Anyone who's studied geometry should be familiar with the concept of a theorem: a relatively
simple rule used to solve a problem, derived from a more intensive analysis using fundamental
rules of mathematics. At least hypothetically, any problem in math can be solved just by using
the simple rules of arithmetic (in fact, this is how modern digital computers carry out the most
complex mathematical calculations: by repeating many cycles of additions and subtractions!),
but human beings aren't as consistent or as fast as a digital computer. We need "shortcut"
methods in order to avoid procedural errors.
In electric network analysis, the fundamental rules are Ohm's Law and Kirchhoff's Laws.
While these humble laws may be applied to analyze just about any circuit configuration (even if
we have to resort to complex algebra to handle multiple unknowns), there are some "shortcut"
methods of analysis to make the math easier for the average human.
As with any theorem of geometry or algebra, these network theorems are derived from
fundamental rules. In this chapter, I'm not going to delve into the formal proofs of any of
these theorems. If you doubt their validity, you can always empirically test them by setting
up example circuits and calculating values using the "old" (simultaneous equation) methods
versus the "new" theorems, to see if the answers coincide. They always should!
10.6
Millman's Theorem
In Millman's Theorem, the circuit is re-drawn as a parallel network of branches, each branch
containing a resistor or series battery/resistor combination. Millman's Theorem is applicable
only to those circuits which can be re-drawn accordingly. Here again is our example circuit
used for the last two analysis methods:
362
CHAPTER 10. DC NETWORK ANALYSIS
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
B1
B2
And here is that same circuit, re-drawn for the sake of applying Millman's Theorem:
R1
R2
R3
4 Ω
2 Ω
1 Ω
28 V
7 V
B1
B3
By considering the supply voltage within each branch and the resistance within each branch,
Millman's Theorem will tell us the voltage across all branches. Please note that I've labeled
the battery in the rightmost branch as "B3" to clearly denote it as being in the third branch,
even though there is no "B2" in the circuit!
Millman's Theorem is nothing more than a long equation, applied to any circuit drawn
as a set of parallel-connected branches, each branch with its own voltage source and series
resistance:
Millman's Theorem Equation
EB1
R1
EB2
EB3
R2
R3
1
R1
R2
R3
1
1
= Voltage across all branches
Substituting actual voltage and resistance figures from our example circuit for the variable
terms of this equation, we get the following expression:
28 V
4 Ω
0 V
2 Ω
7 V
1 Ω
1
4 Ω
2 Ω
1 Ω
1
1
The final answer of 8 volts is the voltage seen across all parallel branches, like this:
10.6.
MILLMAN'S THEOREM
363
R1
R2
R3
28 V
7 V
8 V
8 V
1 V
20 V
B1
B3
The polarity of all voltages in Millman's Theorem are referenced to the same point. In the
example circuit above, I used the bottom wire of the parallel circuit as my reference point,
and so the voltages within each branch (28 for the R1 branch, 0 for the R2 branch, and 7
for the R3 branch) were inserted into the equation as positive numbers. Likewise, when the
answer came out to 8 volts (positive), this meant that the top wire of the circuit was positive
with respect to the bottom wire (the original point of reference). If both batteries had been
connected backwards (negative ends up and positive ends down), the voltage for branch 1
would have been entered into the equation as a -28 volts, the voltage for branch 3 as -7 volts,
and the resulting answer of -8 volts would have told us that the top wire was negative with
respect to the bottom wire (our initial point of reference).
To solve for resistor voltage drops, the Millman voltage (across the parallel network) must
be compared against the voltage source within each branch, using the principle of voltages
adding in series to determine the magnitude and polarity of voltage across each resistor:
ER2 = 8 V - 0 V = 8 V (positive on top)
ER3 = 8 V - 7 V = 1 V (positive on top)
To solve for branch currents, each resistor voltage drop can be divided by its respective
4 Ω
8 V
2 Ω
1 V
1 Ω
The direction of current through each resistor is determined by the polarity across each
resistor, not by the polarity across each battery, as current can be forced backwards through a
battery, as is the case with B3 in the example circuit. This is important to keep in mind, since
Millman's Theorem doesn't provide as direct an indication of "wrong" current direction as does
364
CHAPTER 10. DC NETWORK ANALYSIS
the Branch Current or Mesh Current methods. You must pay close attention to the polarities
of resistor voltage drops as given by Kirchhoff's Voltage Law, determining direction of currents
from that.
R1
R2
R3
28 V
7 V
8 V
20 V
1 V
IR3
IR2
IR1
5 A
4 A
1 A
B1
B3
Millman's Theorem is very convenient for determining the voltage across a set of parallel
branches, where there are enough voltage sources present to preclude solution via regular
series-parallel reduction method. It also is easy in the sense that it doesn't require the use of
simultaneous equations. However, it is limited in that it only applied to circuits which can be
re-drawn to fit this form. It cannot be used, for example, to solve an unbalanced bridge circuit.
And, even in cases where Millman's Theorem can be applied, the solution of individual resistor
voltage drops can be a bit daunting to some, the Millman's Theorem equation only providing a
single figure for branch voltage.
As you will see, each network analysis method has its own advantages and disadvantages.
Each method is a tool, and there is no tool that is perfect for all jobs. The skilled technician,
however, carries these methods in his or her mind like a mechanic carries a set of tools in his
or her tool box. The more tools you have equipped yourself with, the better prepared you will
be for any eventuality.
• REVIEW:
• Millman's Theorem treats circuits as a parallel set of series-component branches.
• All voltages entered and solved for in Millman's Theorem are polarity-referenced at the
same point in the circuit (typically the bottom wire of the parallel network).
10.7
Superposition Theorem
Superposition theorem is one of those strokes of genius that takes a complex subject and sim-
plifies it in a way that makes perfect sense. A theorem like Millman's certainly works well, but
it is not quite obvious why it works so well. Superposition, on the other hand, is obvious.
The strategy used in the Superposition Theorem is to eliminate all but one source of power
within a network at a time, using series/parallel analysis to determine voltage drops (and/or
currents) within the modified network for each power source separately. Then, once voltage
drops and/or currents have been determined for each power source working separately, the
values are all "superimposed" on top of each other (added algebraically) to find the actual
10.7.
SUPERPOSITION THEOREM
365
voltage drops/currents with all sources active. Let's look at our example circuit again and
apply Superposition Theorem to it:
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
B1
B2
Since we have two sources of power in this circuit, we will have to calculate two sets of
values for voltage drops and/or currents, one for the circuit with only the 28 volt battery in
effect. . .
R1
4 Ω
28 V
R2
2 Ω
R3
1 Ω
B1
. . . and one for the circuit with only the 7 volt battery in effect:
7 V
R1
R2
R3
4 Ω
2 Ω
1 Ω
B2
When re-drawing the circuit for series/parallel analysis with one source, all other voltage
sources are replaced by wires (shorts), and all current sources with open circuits (breaks).
Since we only have voltage sources (batteries) in our example circuit, we will replace every
inactive source during analysis with a wire.
Analyzing the circuit with only the 28 volt battery, we obtain the following values for voltage
and current:
366
CHAPTER 10. DC NETWORK ANALYSIS
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
28
24
4
4
4
4
2
1
0.667
4.667
6
6
6
2
4
R1
28 V
R2
R3
24 V
4 V
4 V
6 A
4 A
2 A
B1
Analyzing the circuit with only the 7 volt battery, we obtain another set of values for voltage
and current:
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
4
2
1
R3
7
1.333
2.333
3
3
3
1
2
4
4
3
4
R1
R2
R3
4 V
4 V
3 V
7 V
1 A
2 A
3 A
B2
When superimposing these values of voltage and current, we have to be very careful to
consider polarity (voltage drop) and direction (electron flow), as the values have to be added
algebraically.
10.7.
SUPERPOSITION THEOREM
367
With 28 V
battery
With 7 V
battery
With both batteries
24 V
ER1
ER2
ER3
4 V
20 V
4 V
4 V
8 V
4 V
3 V
1 V
ER1
ER1
ER3
ER3
ER2
ER2
Applying these superimposed voltage figures to the circuit, the end result looks something
like this:
R1
R2
R3
28 V
7 V
20 V
8 V
1 V
B1
B2
Currents add up algebraically as well, and can either be superimposed as done with the
resistor voltage drops, or simply calculated from the final voltage drops and respective resis-
tances (I=E/R). Either way, the answers will be the same. Here I will show the superposition
method applied to current:
368
CHAPTER 10. DC NETWORK ANALYSIS
With 28 V
battery
With 7 V
battery
With both batteries
6 A
IR1
IR1
1 A
IR1
5 A
2 A
2 A
4 A
IR2
IR2
IR2
4 A
3 A
IR3
IR3
IR3
1 A
Once again applying these superimposed figures to our circuit:
R1
R2
R3
28 V
7 V
5 A
4 A
1 A
B1
B2
Quite simple and elegant, don't you think? It must be noted, though, that the Superposition
Theorem works only for circuits that are reducible to series/parallel combinations for each of
the power sources at a time (thus, this theorem is useless for analyzing an unbalanced bridge
circuit), and it only works where the underlying equations are linear (no mathematical powers
or roots). The requisite of linearity means that Superposition Theorem is only applicable for
determining voltage and current, not power!!! Power dissipations, being nonlinear functions,
do not algebraically add to an accurate total when only one source is considered at a time. The
need for linearity also means this Theorem cannot be applied in circuits where the resistance
of a component changes with voltage or current. Hence, networks containing components like
lamps (incandescent or gas-discharge) or varistors could not be analyzed.
Another prerequisite for Superposition Theorem is that all components must be "bilateral,"
meaning that they behave the same with electrons flowing either direction through them. Re-
sistors have no polarity-specific behavior, and so the circuits we've been studying so far all
meet this criterion.
The Superposition Theorem finds use in the study of alternating current (AC) circuits, and
10.8.
THEVENIN'S THEOREM
369
semiconductor (amplifier) circuits, where sometimes AC is often mixed (superimposed) with
DC. Because AC voltage and current equations (Ohm's Law) are linear just like DC, we can
use Superposition to analyze the circuit with just the DC power source, then just the AC power
source, combining the results to tell what will happen with both AC and DC sources in ef-
fect. For now, though, Superposition will suffice as a break from having to do simultaneous
equations to analyze a circuit.
• REVIEW:
• The Superposition Theorem states that a circuit can be analyzed with only one source of
power at a time, the corresponding component voltages and currents algebraically added
to find out what they'll do with all power sources in effect.
• To negate all but one power source for analysis, replace any source of voltage (batteries)
with a wire; replace any current source with an open (break).
10.8
Thevenin's Theorem
Thevenin's Theorem states that it is possible to simplify any linear circuit, no matter how
complex, to an equivalent circuit with just a single voltage source and series resistance con-
nected to a load. The qualification of "linear" is identical to that found in the Superposition
Theorem, where all the underlying equations must be linear (no exponents or roots). If we're
dealing with passive components (such as resistors, and later, inductors and capacitors), this is
true. However, there are some components (especially certain gas-discharge and semiconduc-
tor components) which are nonlinear: that is, their opposition to current changes with voltage
and/or current. As such, we would call circuits containing these types of components, nonlinear
circuits.
Thevenin's Theorem is especially useful in analyzing power systems and other circuits
where one particular resistor in the circuit (called the "load" resistor) is subject to change, and
re-calculation of the circuit is necessary with each trial value of load resistance, to determine
voltage across it and current through it. Let's take another look at our example circuit:
28 V
7 V
2 Ω
R2
R1
R3
4 Ω
1 Ω
B1
B2
Let's suppose that we decide to designate R2 as the "load" resistor in this circuit. We already
have four methods of analysis at our disposal (Branch Current, Mesh Current, Millman's Theo-
rem, and Superposition Theorem) to use in determining voltage across R2 and current through
R2, but each of these methods are time-consuming. Imagine repeating any of these meth-
ods over and over again to find what would happen if the load resistance changed (changing
370
CHAPTER 10. DC NETWORK ANALYSIS
load resistance is very common in power systems, as multiple loads get switched on and off as
needed. the total resistance of their parallel connections changing depending on how many are
connected at a time). This could potentially involve a lot of work!
Thevenin's Theorem makes this easy by temporarily removing the load resistance from the
original circuit and reducing what's left to an equivalent circuit composed of a single voltage
source and series resistance. The load resistance can then be re-connected to this "Thevenin
equivalent circuit" and calculations carried out as if the whole network were nothing but a
simple series circuit:
R1
R2
(Load)
R3
28 V
7 V
4 Ω
2 Ω
1 Ω
B1
B2
. . . after Thevenin conversion . . .
RThevenin
R2
(Load)
EThevenin
Thevenin Equivalent Circuit
2 Ω
The "Thevenin Equivalent Circuit" is the electrical equivalent of B1, R1, R3, and B2 as seen
from the two points where our load resistor (R2) connects.
The Thevenin equivalent circuit, if correctly derived, will behave exactly the same as the
original circuit formed by B1, R1, R3, and B2. In other words, the load resistor (R2) voltage and
current should be exactly the same for the same value of load resistance in the two circuits.
The load resistor R2 cannot "tell the difference" between the original network of B1, R1, R3, and
B2, and the Thevenin equivalent circuit of ET hevenin, and RT hevenin, provided that the values
for ET hevenin and RT hevenin have been calculated correctly.
The advantage in performing the "Thevenin conversion" to the simpler circuit, of course,
is that it makes load voltage and load current so much easier to solve than in the original
network. Calculating the equivalent Thevenin source voltage and series resistance is actually
quite easy. First, the chosen load resistor is removed from the original circuit, replaced with a
10.8.
THEVENIN'S THEOREM
371
break (open circuit):
R1
R3
28 V
7 V
Load resistor
removed
4 Ω
1 Ω
B1
B2
Next, the voltage between the two points where the load resistor used to be attached is
determined. Use whatever analysis methods are at your disposal to do this. In this case, the
original circuit with the load resistor removed is nothing more than a simple series circuit
with opposing batteries, and so we can determine the voltage across the open load terminals
by applying the rules of series circuits, Ohm's Law, and Kirchhoff's Voltage Law:
E
I
R
Volts
Amps
Ohms
Total
R1
R3
4
1
5
21
4.2
4.2
4.2
16.8
4.2
R1
R3
28 V
7 V
4 Ω
1 Ω
16.8 V
4.2 V
4.2 A
4.2 A
11.2 V
B1
B2
The voltage between the two load connection points can be figured from the one of the
battery's voltage and one of the resistor's voltage drops, and comes out to 11.2 volts. This is our
"Thevenin voltage" (ET hevenin) in the equivalent circuit:
372
CHAPTER 10. DC NETWORK ANALYSIS
RThevenin
R2
(Load)
EThevenin
Thevenin Equivalent Circuit
2 Ω
11.2 V
To find the Thevenin series resistance for our equivalent circuit, we need to take the original
circuit (with the load resistor still removed), remove the power sources (in the same style as we
did with the Superposition Theorem: voltage sources replaced with wires and current sources
replaced with breaks), and figure the resistance from one load terminal to the other:
R1
R3
4 Ω
1 Ω
0.8 Ω
With the removal of the two batteries, the total resistance measured at this location is equal
to R1 and R3 in parallel: 0.8 Ω. This is our "Thevenin resistance" (RT hevenin) for the equivalent
circuit:
RThevenin
R2
(Load)
EThevenin
Thevenin Equivalent Circuit
2 Ω
11.2 V
0.8 Ω
10.9.
NORTON'S THEOREM
373
With the load resistor (2 Ω) attached between the connection points, we can determine
voltage across it and current through it as though the whole network were nothing more than
a simple series circuit:
E
I
R
Volts
Amps
Ohms
Total
RThevenin
RLoad
11.2
4
2.8
0.8
2
4
4
3.2
8
Notice that the voltage and current figures for R2 (8 volts, 4 amps) are identical to those
found using other methods of analysis. Also notice that the voltage and current figures for the
Thevenin series resistance and the Thevenin source (total) do not apply to any component in
the original, complex circuit. Thevenin's Theorem is only useful for determining what happens
to a single resistor in a network: the load.
The advantage, of course, is that you can quickly determine what would happen to that
single resistor if it were of a value other than 2 Ωwithout having to go through a lot of analysis
again. Just plug in that other value for the load resistor into the Thevenin equivalent circuit
and a little bit of series circuit calculation will give you the result.
• REVIEW:
• Thevenin's Theorem is a way to reduce a network to an equivalent circuit composed of a
single voltage source, series resistance, and series load.
• Steps to follow for Thevenin's Theorem:
• (1) Find the Thevenin source voltage by removing the load resistor from the original
circuit and calculating voltage across the open connection points where the load resistor
used to be.
• (2) Find the Thevenin resistance by removing all power sources in the original circuit
(voltage sources shorted and current sources open) and calculating total resistance be-
tween the open connection points.
• (3) Draw the Thevenin equivalent circuit, with the Thevenin voltage source in series with
the Thevenin resistance. The load resistor re-attaches between the two open points of the
equivalent circuit.
• (4) Analyze voltage and current for the load resistor following the rules for series circuits.
10.9
Norton's Theorem
Norton's Theorem states that it is possible to simplify any linear circuit, no matter how com-
plex, to an equivalent circuit with just a single current source and parallel resistance connected
to a load. Just as with Thevenin's Theorem, the qualification of "linear" is identical to that
found in the Superposition Theorem: all underlying equations must be linear (no exponents or
roots).
374
CHAPTER 10. DC NETWORK ANALYSIS
Contrasting our original example circuit against the Norton equivalent: it looks something
like this:
R1
R2
(Load)
R3
28 V
7 V
4 Ω
2 Ω
1 Ω
B1
B2
. . . after Norton conversion . . .
INorton
RNorton
R2
2 Ω
(Load)
Norton Equivalent Circuit
Remember that a current source is a component whose job is to provide a constant amount
of current, outputting as much or as little voltage necessary to maintain that constant current.
As with Thevenin's Theorem, everything in the original circuit except the load resistance
has been reduced to an equivalent circuit that is simpler to analyze. Also similar to Thevenin's
Theorem are the steps used in Norton's Theorem to calculate the Norton source current (INorton)
and Norton resistance (RNorton).
As before, the first step is to identify the load resistance and remove it from the original
circuit:
R1
R3
28 V
7 V
Load resistor
removed
4 Ω
1 Ω
B1
B2
Then, to find the Norton current (for the current source in the Norton equivalent circuit),
10.9.
NORTON'S THEOREM
375
place a direct wire (short) connection between the load points and determine the resultant
current. Note that this step is exactly opposite the respective step in Thevenin's Theorem,
where we replaced the load resistor with a break (open circuit):
R1
R3
28 V
7 V
4 Ω
1 Ω
7 A
7 A
14 A
B1
B2
With zero voltage dropped between the load resistor connection points, the current through
R1 is strictly a function of B1's voltage and R1's resistance: 7 amps (I=E/R). Likewise, the
current through R3 is now strictly a function of B2's voltage and R3's resistance: 7 amps (I=E/R).
The total current through the short between the load connection points is the sum of these two
currents: 7 amps + 7 amps = 14 amps. This figure of 14 amps becomes the Norton source
current (INorton) in our equivalent circuit:
INorton
RNorton
R2
2 Ω
(Load)
14 A
Norton Equivalent Circuit
Remember, the arrow notation for a current source points in the direction opposite that
of electron flow. Again, apologies for the confusion. For better or for worse, this is standard
electronic symbol notation. Blame Mr. Franklin again!
To calculate the Norton resistance (RNorton), we do the exact same thing as we did for
calculating Thevenin resistance (RT hevenin): take the original circuit (with the load resistor
still removed), remove the power sources (in the same style as we did with the Superposition
Theorem: voltage sources replaced with wires and current sources replaced with breaks), and
figure total resistance from one load connection point to the other:
376
CHAPTER 10. DC NETWORK ANALYSIS
R1
R3
4 Ω
1 Ω
0.8 Ω
Now our Norton equivalent circuit looks like this:
INorton
RNorton
R2
2 Ω
(Load)
14 A
0.8 Ω
Norton Equivalent Circuit
If we re-connect our original load resistance of 2 Ω, we can analyze the Norton circuit as a
simple parallel arrangement:
E
I
R
Volts
Amps
Ohms
Total
RLoad
0.8
2
4
8
RNorton
8
8
14
10
571.43m
As with the Thevenin equivalent circuit, the only useful information from this analysis is
the voltage and current values for R2; the rest of the information is irrelevant to the original
circuit. However, the same advantages seen with Thevenin's Theorem apply to Norton's as
well: if we wish to analyze load resistor voltage and current over several different values of
load resistance, we can use the Norton equivalent circuit again and again, applying nothing
more complex than simple parallel circuit analysis to determine what's happening with each
trial load.
• REVIEW:
• Norton's Theorem is a way to reduce a network to an equivalent circuit composed of a
single current source, parallel resistance, and parallel load.
• Steps to follow for Norton's Theorem:
10.10.
THEVENIN-NORTON EQUIVALENCIES
377
• (1) Find the Norton source current by removing the load resistor from the original circuit
and calculating current through a short (wire) jumping across the open connection points
where the load resistor used to be.
• (2) Find the Norton resistance by removing all power sources in the original circuit (volt-
age sources shorted and current sources open) and calculating total resistance between
the open connection points.
• (3) Draw the Norton equivalent circuit, with the Norton current source in parallel with
the Norton resistance. The load resistor re-attaches between the two open points of the
equivalent circuit.
• (4) Analyze voltage and current for the load resistor following the rules for parallel cir-
cuits.
10.10
Thevenin-Norton equivalencies
Since Thevenin's and Norton's Theorems are two equally valid methods of reducing a complex
network down to something simpler to analyze, there must be some way to convert a Thevenin
equivalent circuit to a Norton equivalent circuit, and vice versa (just what you were dying to
know, right?). Well, the procedure is very simple.
You may have noticed that the procedure for calculating Thevenin resistance is identical
to the procedure for calculating Norton resistance: remove all power sources and determine
resistance between the open load connection points. As such, Thevenin and Norton resistances
for the same original network must be equal. Using the example circuits from the last two
sections, we can see that the two resistances are indeed equal:
RThevenin
R2
(Load)
EThevenin
Thevenin Equivalent Circuit
2 Ω
11.2 V
0.8 Ω
378
CHAPTER 10. DC NETWORK ANALYSIS
INorton
RNorton
R2
2 Ω
(Load)
14 A
0.8 Ω
Norton Equivalent Circuit
RThevenin = RNorton
Considering the fact that both Thevenin and Norton equivalent circuits are intended to
behave the same as the original network in supplying voltage and current to the load resistor
(as seen from the perspective of the load connection points), these two equivalent circuits,
having been derived from the same original network should behave identically.
This means that both Thevenin and Norton equivalent circuits should produce the same
voltage across the load terminals with no load resistor attached. With the Thevenin equivalent,
the open-circuited voltage would be equal to the Thevenin source voltage (no circuit current
present to drop voltage across the series resistor), which is 11.2 volts in this case. With the
Norton equivalent circuit, all 14 amps from the Norton current source would have to flow
through the 0.8 ΩNorton resistance, producing the exact same voltage, 11.2 volts (E=IR).
Thus, we can say that the Thevenin voltage is equal to the Norton current times the Norton
resistance:
EThevenin = INortonRNorton
So, if we wanted to convert a Norton equivalent circuit to a Thevenin equivalent circuit, we
could use the same resistance and calculate the Thevenin voltage with Ohm's Law.
Conversely, both Thevenin and Norton equivalent circuits should generate the same amount
of current through a short circuit across the load terminals. With the Norton equivalent, the
short-circuit current would be exactly equal to the Norton source current, which is 14 amps
in this case. With the Thevenin equivalent, all 11.2 volts would be applied across the 0.8 Ω
Thevenin resistance, producing the exact same current through the short, 14 amps (I=E/R).
Thus, we can say that the Norton current is equal to the Thevenin voltage divided by the
Thevenin resistance:
INorton =
EThevenin
RThevenin
This equivalence between Thevenin and Norton circuits can be a useful tool in itself, as we
shall see in the next section.
• REVIEW:
• Thevenin and Norton resistances are equal.
10.11.
MILLMAN'S THEOREM REVISITED
379
• Thevenin voltage is equal to Norton current times Norton resistance.
• Norton current is equal to Thevenin voltage divided by Thevenin resistance.
10.11
Millman's Theorem revisited
You may have wondered where we got that strange equation for the determination of "Millman
Voltage" across parallel branches of a circuit where each branch contains a series resistance
and voltage source:
Millman's Theorem Equation
EB1
R1
EB2
EB3
R2
R3
1
R1
R2
R3
1
1
= Voltage across all branches
Parts of this equation seem familiar to equations we've seen before. For instance, the de-
nominator of the large fraction looks conspicuously like the denominator of our parallel resis-
tance equation. And, of course, the E/R terms in the numerator of the large fraction should
give figures for current, Ohm's Law being what it is (I=E/R).
Now that we've covered Thevenin and Norton source equivalencies, we have the tools neces-
sary to understand Millman's equation. What Millman's equation is actually doing is treating
each branch (with its series voltage source and resistance) as a Thevenin equivalent circuit
and then converting each one into equivalent Norton circuits.
R1
R2
R3
4 Ω
2 Ω
1 Ω
28 V
7 V
B1
B3
Thus, in the circuit above, battery B1 and resistor R1 are seen as a Thevenin source to be
converted into a Norton source of 7 amps (28 volts / 4 Ω) in parallel with a 4 Ωresistor. The
rightmost branch will be converted into a 7 amp current source (7 volts / 1 Ω) and 1 Ωresistor
in parallel. The center branch, containing no voltage source at all, will be converted into a
Norton source of 0 amps in parallel with a 2 Ωresistor:
380
CHAPTER 10. DC NETWORK ANALYSIS
7 A
4 Ω
0 A
2 Ω
7 A
1 Ω
Since current sources directly add their respective currents in parallel, the total circuit
current will be 7 + 0 + 7, or 14 amps. This addition of Norton source currents is what's being
represented in the numerator of the Millman equation:
EB1
R1
EB2
EB3
R2
R3
1
R1
R2
R3
1
1
EB1
R1
EB2
R2
EB3
R3
Millman's Theorem Equation
All the Norton resistances are in parallel with each other as well in the equivalent circuit,
so they diminish to create a total resistance. This diminishing of source resistances is what's
being represented in the denominator of the Millman's equation:
EB1
R1
EB2
EB3
R2
R3
1
R1
R2
R3
1
1
Millman's Theorem Equation
1
R1
1
R2
1
R3
1
In this case, the resistance total will be equal to 571.43 milliohms (571.43 mΩ). We can
re-draw our equivalent circuit now as one with a single Norton current source and Norton
resistance:
14 A
571.43 mΩ
Ohm's Law can tell us the voltage across these two components now (E=IR):
Etotal = (14 A)(571.43 mΩ)
Etotal = 8 V
10.12.
MAXIMUM POWER TRANSFER THEOREM
381
14 A
571.43 mΩ
8 V
Let's summarize what we know about the circuit thus far. We know that the total current in
this circuit is given by the sum of all the branch voltages divided by their respective resistances.
We also know that the total resistance is found by taking the reciprocal of all the branch
resistance reciprocals. Furthermore, we should be well aware of the fact that total voltage
across all the branches can be found by multiplying total current by total resistance (E=IR).
All we need to do is put together the two equations we had earlier for total circuit current and
total resistance, multiplying them to find total voltage:
Ohm's Law:
(total current) x (total resistance) = (total voltage)
EB1
R1
EB2
EB3
R2
R3
1
R1
1
R2
1
R3
1
x
I
R
E
x
= (total voltage)
. . . or . . .
1
R2
1
R1
1
R3
= (total voltage)
EB1
R1
EB2
R2
EB3
R3
The Millman's equation is nothing more than a Thevenin-to-Norton conversion matched
together with the parallel resistance formula to find total voltage across all the branches of the
circuit. So, hopefully some of the mystery is gone now!
10.12
Maximum Power Transfer Theorem
The Maximum Power Transfer Theorem is not so much a means of analysis as it is an aid
to system design. Simply stated, the maximum amount of power will be dissipated by a load
resistance when that load resistance is equal to the Thevenin/Norton resistance of the net-
work supplying the power. If the load resistance is lower or higher than the Thevenin/Norton
resistance of the source network, its dissipated power will be less than maximum.
This is essentially what is aimed for in radio transmitter design , where the antenna or
382
CHAPTER 10. DC NETWORK ANALYSIS
transmission line "impedance" is matched to final power amplifier "impedance" for maximum
radio frequency power output. Impedance, the overall opposition to AC and DC current, is very
similar to resistance, and must be equal between source and load for the greatest amount of
power to be transferred to the load. A load impedance that is too high will result in low power
output. A load impedance that is too low will not only result in low power output, but possibly
overheating of the amplifier due to the power dissipated in its internal (Thevenin or Norton)
impedance.
Taking our Thevenin equivalent example circuit, the Maximum Power Transfer Theorem
tells us that the load resistance resulting in greatest power dissipation is equal in value to the
Thevenin resistance (in this case, 0.8 Ω):
EThevenin
RThevenin
11.2 V
0.8 Ω
RLoad
0.8 Ω
With this value of load resistance, the dissipated power will be 39.2 watts:
E
I
R
Volts
Amps
Ohms
Total
RLoad
RThevenin
P
Watts
11.2
0.8
0.8
1.6
7
7
7
5.6
5.6
39.2
39.2
78.4
If we were to try a lower value for the load resistance (0.5 Ωinstead of 0.8 Ω, for example),
our power dissipated by the load resistance would decrease:
E
I
R
Volts
Amps
Ohms
Total
RLoad
RThevenin
P
Watts
11.2
0.8
0.5
1.3
8.615
8.615
8.615
6.892
4.308
59.38
37.11
96.49
Power dissipation increased for both the Thevenin resistance and the total circuit, but it
decreased for the load resistor. Likewise, if we increase the load resistance (1.1 Ωinstead of
0.8 Ω, for example), power dissipation will also be less than it was at 0.8 Ωexactly:
10.13.
∆-Y AND Y-∆CONVERSIONS
383
E
I
R
Volts
Amps
Ohms
Total
RLoad
RThevenin
P
Watts
11.2
0.8
1.1
1.9
5.895
5.895
5.895
4.716
6.484
27.80
38.22
66.02
If you were designing a circuit for maximum power dissipation at the load resistance, this
theorem would be very useful. Having reduced a network down to a Thevenin voltage and
resistance (or Norton current and resistance), you simply set the load resistance equal to that
Thevenin or Norton equivalent (or vice versa) to ensure maximum power dissipation at the
load. Practical applications of this might include radio transmitter final amplifier stage design
(seeking to maximize power delivered to the antenna or transmission line), a grid tied inverter
loading a solar array, or electric vehicle design (seeking to maximize power delivered to drive
motor).
The Maximum Power Transfer Theorem is not: Maximum power transfer does not
coincide with maximum efficiency. Application of The Maximum Power Transfer theorem to
AC power distribution will not result in maximum or even high efficiency. The goal of high ef-
ficiency is more important for AC power distribution, which dictates a relatively low generator
impedance compared to load impedance.
Similar to AC power distribution, high fidelity audio amplifiers are designed for a relatively
low output impedance and a relatively high speaker load impedance. As a ratio, "output imp-
dance" : "load impedance" is known as damping factor, typically in the range of 100 to 1000.
Maximum power transfer does not coincide with the goal of lowest noise. For example, the
low-level radio frequency amplifier between the antenna and a radio receiver is often designed
for lowest possible noise. This often requires a mismatch of the amplifier input impedance to
the antenna as compared with that dictated by the maximum power transfer theorem.
• REVIEW:
• The Maximum Power Transfer Theorem states that the maximum amount of power will
be dissipated by a load resistance if it is equal to the Thevenin or Norton resistance of
the network supplying power.
• The Maximum Power Transfer Theorem does not satisfy the goal of maximum efficiency.
10.13
∆-Y and Y-∆conversions
In many circuit applications, we encounter components connected together in one of two ways
to form a three-terminal network: the "Delta," or ∆(also known as the "Pi," or π) configuration,
and the "Y" (also known as the "T") configuration.
384
CHAPTER 10. DC NETWORK ANALYSIS
A
B
C
B
C
A
RAC
RAB
RBC
RA
RC
RB
Delta (∆) network
Wye (Y) network
A
C
B
A
C
B
RAC
RAB
RBC
RA
RC
RB
Tee (T) network
Pi (π) network
It is possible to calculate the proper values of resistors necessary to form one kind of net-
work (∆or Y) that behaves identically to the other kind, as analyzed from the terminal con-
nections alone. That is, if we had two separate resistor networks, one ∆and one Y, each with
its resistors hidden from view, with nothing but the three terminals (A, B, and C) exposed for
testing, the resistors could be sized for the two networks so that there would be no way to
electrically determine one network apart from the other. In other words, equivalent ∆and Y
networks behave identically.
There are several equations used to convert one network to the other:
To convert a Delta (∆) to a Wye (Y)
To convert a Wye (Y) to a Delta (∆)
RAB RAC
RAB RBC
RAC RBC
RC
RA
RB
∆and Y networks are seen frequently in 3-phase AC power systems (a topic covered in
volume II of this book series), but even then they're usually balanced networks (all resistors
10.13.
∆-Y AND Y-∆CONVERSIONS
385
equal in value) and conversion from one to the other need not involve such complex calcula-
tions. When would the average technician ever need to use these equations?
A prime application for ∆-Y conversion is in the solution of unbalanced bridge circuits, such
as the one below:
10 V
R1
12 Ω
R2
18 Ω
R3
6 Ω
R4
18 Ω
12 Ω
R5
Solution of this circuit with Branch Current or Mesh Current analysis is fairly involved,
and neither the Millman nor Superposition Theorems are of any help, since there's only one
source of power. We could use Thevenin's or Norton's Theorem, treating R3 as our load, but
what fun would that be?
If we were to treat resistors R1, R2, and R3 as being connected in a ∆configuration (Rab,
Rac, and Rbc, respectively) and generate an equivalent Y network to replace them, we could
turn this bridge circuit into a (simpler) series/parallel combination circuit:
R5
10 V
12 Ω
18 Ω
6 Ω
R4
18 Ω
12 Ω
Selecting Delta (∆) network to convert:
A
B
C
RAB
RAC
RBC
After the ∆-Y conversion . . .
386
CHAPTER 10. DC NETWORK ANALYSIS
A
B
C
10 V
RA
RB
RC
R4
R5
18 Ω
12 Ω
∆ converted to a Y
If we perform our calculations correctly, the voltages between points A, B, and C will be the
same in the converted circuit as in the original circuit, and we can transfer those values back
to the original bridge configuration.
(12 Ω)(18 Ω)
36
36
(12 Ω)(6 Ω)
72
2 Ω
36
RC
(18 Ω)(6 Ω)
108
3 Ω
A
B
C
10 V
RA
RB
RC
R4
R5
18 Ω
12 Ω
2 Ω
3 Ω
6 Ω
10.13.
∆-Y AND Y-∆CONVERSIONS
387
Resistors R4 and R5, of course, remain the same at 18 Ωand 12 Ω, respectively. Analyzing
the circuit now as a series/parallel combination, we arrive at the following figures:
E
I
R
Volts
Amps
Ohms
RA
RB
RC
R4
R5
6
2
3
18
12
E
I
R
Volts
Amps
Ohms
Total
20
15
8.571
14.571
10
686.27m
686.27m
4.118
5.882
5.882
5.882
294.12m
392.16m
686.27m
294.12m
294.12m
392.16m
392.16m
588.24m
1.176
5.294
4.706
We must use the voltage drops figures from the table above to determine the voltages be-
tween points A, B, and C, seeing how the add up (or subtract, as is the case with voltage
between points B and C):
A
B
C
10 V
R4 R5
RB
RC
RA
4.118 V
0.588
V
1.176
V
5.294
V
4.706
V
0.588
V
4.706 V
5.294 V
Now that we know these voltages, we can transfer them to the same points A, B, and C in
the original bridge circuit:
388
CHAPTER 10. DC NETWORK ANALYSIS
10 V
R1 R2
R3
R4 R5
4.706
V
5.294
V
5.294
V
4.706
V
0.588 V
Voltage drops across R4 and R5, of course, are exactly the same as they were in the converted
circuit.
At this point, we could take these voltages and determine resistor currents through the
repeated use of Ohm's Law (I=E/R):
IR1 = 4.706 V
12 Ω
= 392.16 mA
IR2 = 5.294 V
18 Ω
= 294.12 mA
IR3 = 588.24 mV
6 Ω
5.294 V
18 Ω
= 294.12 mA
12 Ω
4.706 V
= 392.16 mA
A quick simulation with SPICE will serve to verify our work:[2]
10 V
R1
12 Ω
R2
18 Ω
R3
6 Ω
R4
18 Ω
12 Ω
R5
1
0
0
1
2
3
unbalanced bridge circuit
v1 1 0
10.14.
CONTRIBUTORS
389
r1 1 2 12
r2 1 3 18
r3 2 3 6
r4 2 0 18
r5 3 0 12
.dc v1 10 10 1
.print dc v(1,2) v(1,3) v(2,3) v(2,0) v(3,0)
.end
v1
v(1,2)
v(1,3)
v(2,3)
v(2)
v(3)
1.000E+01
4.706E+00
5.294E+00
5.882E-01
5.294E+00
4.706E+00
The voltage figures, as read from left to right, represent voltage drops across the five respec-
tive resistors, R1 through R5. I could have shown currents as well, but since that would have
required insertion of "dummy" voltage sources in the SPICE netlist, and since we're primarily
interested in validating the ∆-Y conversion equations and not Ohm's Law, this will suffice.
• REVIEW:
• "Delta" (∆) networks are also known as "Pi" (π) networks.
• "Y" networks are also known as "T" networks.
• ∆and Y networks can be converted to their equivalent counterparts with the proper
resistance equations. By "equivalent," I mean that the two networks will be electrically
identical as measured from the three terminals (A, B, and C).
• A bridge circuit can be simplified to a series/parallel circuit by converting half of it from a
∆to a Y network. After voltage drops between the original three connection points (A, B,
and C) have been solved for, those voltages can be transferred back to the original bridge
circuit, across those same equivalent points.
10.14
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Dejan Budimir (January 2003): Suggested clarifications for explaining the Mesh Current
method of circuit analysis.
Bill Heath (December 2002): Pointed out several typographical errors.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Davy Van Nieuwenborgh (April 2004): Pointed out error in Mesh current section, sup-
plied editorial material, end of section.
390
CHAPTER 10. DC NETWORK ANALYSIS
Bibliography
[1] A.E. Fitzergerald, David E. Higginbotham, Arvin Grabel, Basic Electrical Engineering,
(McGraw-Hill, 1975).
Kuphaldt,Using
the
Spice
Circuit
Simulation
Program,
in"Lessons
in
Electricity,
Reference",
Volume
5,
Chapter
7,
at
[3] Davy Van Nieuwenborgh, private communications, Theoretical Computer Science labora-
tory, Department of Computer Science, Vrije Universiteit Brussel (4/7/2004).
Matrix calculator open source program for Linux or MS Windows,
at
[5] Ray A. Rayburn , private communications, Senior Consultant K2 Audio, LLC; Fellow of
the Audio Engineering Society, (6/29/2009).
power-amplifiers/buying-guide.php#2
Chapter 11
BATTERIES AND POWER
SYSTEMS
Contents
11.1 Electron activity in chemical reactions . . . . . . . . . . . . . . . . . . . . . 391
11.2 Battery construction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 397
11.3 Battery ratings
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 400
11.4 Special-purpose batteries . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 402
11.5 Practical considerations
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 406
11.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 408
Bibliography . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 408
11.1
Electron activity in chemical reactions
So far in our discussions on electricity and electric circuits, we have not discussed in any detail
how batteries function. Rather, we have simply assumed that they produce constant voltage
through some sort of mysterious process. Here, we will explore that process to some degree and
cover some of the practical considerations involved with real batteries and their use in power
systems.
In the first chapter of this book, the concept of an atom was discussed, as being the basic
building-block of all material objects. Atoms, in turn, are composed of even smaller pieces
of matter called particles. Electrons, protons, and neutrons are the basic types of particles
found in atoms. Each of these particle types plays a distinct role in the behavior of an atom.
While electrical activity involves the motion of electrons, the chemical identity of an atom
(which largely determines how conductive the material will be) is determined by the number
of protons in the nucleus (center).
391
392
CHAPTER 11. BATTERIES AND POWER SYSTEMS
= electron
= neutron
e
N
P
P
P
P
P
P
P
N
N
N
N
N
N
e
e
e
e
e
e
The protons in an atom's nucleus are extremely difficult to dislodge, and so the chemical
identity of any atom is very stable. One of the goals of the ancient alchemists (to turn lead
into gold) was foiled by this sub-atomic stability. All efforts to alter this property of an atom by
means of heat, light, or friction were met with failure. The electrons of an atom, however, are
much more easily dislodged. As we have already seen, friction is one way in which electrons
can be transferred from one atom to another (glass and silk, wax and wool), and so is heat
(generating voltage by heating a junction of dissimilar metals, as in the case of thermocouples).
Electrons can do much more than just move around and between atoms: they can also serve
to link different atoms together. This linking of atoms by electrons is called a chemical bond.
A crude (and simplified) representation of such a bond between two atoms might look like this:
P
P
P
P
P
P
N
N
N
N
N
N
P
P
P
P
P
P
N
N
N
N
N
N
e
e
e
e
e
e
e
e
e
e
e
e
There are several types of chemical bonds, the one shown above being representative of a
11.1.
ELECTRON ACTIVITY IN CHEMICAL REACTIONS
393
covalent bond, where electrons are shared between atoms. Because chemical bonds are based
on links formed by electrons, these bonds are only as strong as the immobility of the electrons
forming them. That is to say, chemical bonds can be created or broken by the same forces that
force electrons to move: heat, light, friction, etc.
When atoms are joined by chemical bonds, they form materials with unique properties
known as molecules. The dual-atom picture shown above is an example of a simple molecule
formed by two atoms of the same type. Most molecules are unions of different types of atoms.
Even molecules formed by atoms of the same type can have radically different physical proper-
ties. Take the element carbon, for instance: in one form, graphite, carbon atoms link together
to form flat "plates" which slide against one another very easily, giving graphite its natural
lubricating properties. In another form, diamond, the same carbon atoms link together in
a different configuration, this time in the shapes of interlocking pyramids, forming a mate-
rial of exceeding hardness. In yet another form, Fullerene, dozens of carbon atoms form each
molecule, which looks something like a soccer ball. Fullerene molecules are very fragile and
lightweight. The airy soot formed by excessively rich combustion of acetylene gas (as in the
initial ignition of an oxy-acetylene welding/cutting torch) contains many Fullerene molecules.
When alchemists succeeded in changing the properties of a substance by heat, light, friction,
or mixture with other substances, they were really observing changes in the types of molecules
formed by atoms breaking and forming bonds with other atoms. Chemistry is the modern
counterpart to alchemy, and concerns itself primarily with the properties of these chemical
bonds and the reactions associated with them.
A type of chemical bond of particular interest to our study of batteries is the so-called ionic
bond, and it differs from the covalent bond in that one atom of the molecule possesses an excess
of electrons while another atom lacks electrons, the bonds between them being a result of the
electrostatic attraction between the two unlike charges. When ionic bonds are formed from
neutral atoms, there is a transfer of electrons between the positively and negatively charged
atoms. An atom that gains an excess of electrons is said to be reduced; an atom with a defi-
ciency of electrons is said to be oxidized. A mnemonic to help remember the definitions is OIL
RIG (oxidized is less; reduced is gained). It is important to note that molecules will often con-
tain both ionic and covalent bonds. Sodium hydroxide (lye, NaOH) has an ionic bond between
the sodium atom (positive) and the hydroxyl ion (negative). The hydroxyl ion has a covalent
bond (shown as a bar) between the hydrogen and oxygen atoms:
Sodium only loses one electron, so its charge is +1 in the above example. If an atom loses
more than one electron, the resulting charge can be indicated as +2, +3, +4, etc. or by a Roman
numeral in parentheses showing the oxidation state, such as (I), (II), (IV), etc. Some atoms can
have multiple oxidation states, and it is sometimes important to include the oxidation state in
the molecular formula to avoid ambiguity.
The formation of ions and ionic bonds from neutral atoms or molecules (or vice versa) in-
volves the transfer of electrons. That transfer of electrons can be harnessed to generate an
electric current.A device constructed to do just this is called a voltaic cell, or cell for short, usu-
ally consisting of two metal electrodes immersed in a chemical mixture (called an electrolyte)
designed to facilitate such an electrochemical (oxidation/reduction) reaction:
394
CHAPTER 11. BATTERIES AND POWER SYSTEMS
electrolyte solution
electrodes
The two electrodes are made of different materials,
both of which chemically react with the electrolyte
in some form of ionic bonding.
Voltaic cell
In the common "lead-acid" cell (the kind commonly used in automobiles), the negative elec-
trode is made of lead (Pb) and the positive is made of lead (IV) dioxide (Pb02), both metallic
substances. It is important to note that lead dioxide is metallic and is an electrical conductor,
unlike other metal oxides that are usually insulators. (note: Table 11.1) The electrolyte solu-
tion is a dilute sulfuric acid (H2SO4 + H2O). If the electrodes of the cell are connected to an
external circuit, such that electrons have a place to flow from one to the other, lead(IV) atoms
in the positive electrode (PbO2) will gain two electrons each to produce Pb(II)O. The oxygen
atoms which are "left over" combine with positively charged hydrogen ions (H)+to form water
(H2O). This flow of electrons into into the lead dioxide (PbO2) electrode, gives it a positive elec-
trical charge. Consequently, lead atoms in the negative electrode give up two electrons each to
produce lead Pb(II), which combines with sulfate ions (SO4−2) produced from the disassocia-
tion of the hydrogen ions (H+) from the sulfuric acid (H2SO4) to form lead sulfate (PbSO4). The
flow of electrons out of the lead electrode gives it a negative electrical charge. These reactions
are shown diagrammitically below:[1]
11.1.
ELECTRON ACTIVITY IN CHEMICAL REACTIONS
395
Pb electrode
PbO2 electrode
H2SO4 + H2O
electrolyte:
load
I
Pb(II)SO4 + 2H2O
electrons
Lead-acid cell discharging
2PbSO4 + 2H2O
At (+) electrode:
At (-) electrode:
Overall cell:
Table 11.1: Note on lead oxide nomenclature
The nomenclature for lead oxides can be confusing. The term,
lead oxide can refer to either Pb(II)O or Pb(IV)O2, and the
correct compound can be determined usually from context.
Other synonyms for Pb(IV)O2 are: lead dioxide, lead perox-
ide, plumbic oxide, lead oxide brown, and lead superoxide. The
term, lead peroxide is particularly confusing, as it implies a
compound of lead (II) with two oxygen atoms, Pb(II)O2, which
apparently does not exist. Unfortunately, the term lead perox-
ide has persisted in industrial literature. In this section, lead
dioxide will be used to refer to Pb(IV)O2, and lead oxide will re-
fer to Pb(II)O. The oxidation states will not be shown usually.
This process of the cell providing electrical energy to supply a load is called discharging,
since it is depleting its internal chemical reserves. Theoretically, after all of the sulfuric acid
has been exhausted, the result will be two electrodes of lead sulfate (PbSO4) and an electrolyte
solution of pure water (H2O), leaving no more capacity for additional ionic bonding. In this
state, the cell is said to be fully discharged. In a lead-acid cell, the state of charge can be
determined by an analysis of acid strength. This is easily accomplished with a device called
a hydrometer, which measures the specific gravity (density) of the electrolyte. Sulfuric acid is
denser than water, so the greater the charge of a cell, the greater the acid concentration, and
thus a denser electrolyte solution.
There is no single chemical reaction representative of all voltaic cells, so any detailed dis-
cussion of chemistry is bound to have limited application. The important thing to understand
396
CHAPTER 11. BATTERIES AND POWER SYSTEMS
is that electrons are motivated to and/or from the cell's electrodes via ionic reactions between
the electrode molecules and the electrolyte molecules. The reaction is enabled when there is
an external path for electric current, and ceases when that path is broken.
Being that the motivation for electrons to move through a cell is chemical in nature, the
amount of voltage (electromotive force) generated by any cell will be specific to the particular
chemical reaction for that cell type. For instance, the lead-acid cell just described has a nominal
voltage of 2.04 volts per cell, based on a fully "charged" cell (acid concentration strong) in good
physical condition. There are other types of cells with different specific voltage outputs. The
Edison cell, for example, with a positive electrode made of nickel oxide, a negative electrode
made of iron, and an electrolyte solution of potassium hydroxide (a caustic, not acid, substance)
generates a nominal voltage of only 1.2 volts, due to the specific differences in chemical reaction
with those electrode and electrolyte substances.
The chemical reactions of some types of cells can be reversed by forcing electric current
backwards through the cell (in the negative electrode and out the positive electrode). This
process is called charging. Any such (rechargeable) cell is called a secondary cell. A cell whose
chemistry cannot be reversed by a reverse current is called a primary cell.
When a lead-acid cell is charged by an external current source, the chemical reactions ex-
perienced during discharge are reversed:
Pb electrode
PbO2 electrode
H2SO4 + H2O
electrolyte:
I
electrons
Lead-acid cell charging
Gen
Pb(II)SO4 + 2H2O
2PbSO4 + 2H2O
At (+) electrode:
At (-) electrode:
Overall cell:
• REVIEW:
• Atoms bound together by electrons are called molecules.
• Ionic bonds are molecular unions formed when an electron-deficient atom (a positive ion)
joins with an electron-excessive atom (a negative ion).
11.2.
BATTERY CONSTRUCTION
397
• Electrochemical reactions involve the transfer of electrons between atoms. This transfer
can be harnessed to form an electric current.
• A cell is a device constructed to harness such chemical reactions to generate electric cur-
rent.
• A cell is said to be discharged when its internal chemical reserves have been depleted
through use.
• A secondary cell's chemistry can be reversed (recharged) by forcing current backwards
through it.
• A primary cell cannot be practically recharged.
• Lead-acid cell charge can be assessed with an instrument called a hydrometer, which
measures the density of the electrolyte liquid. The denser the electrolyte, the stronger
the acid concentration, and the greater charge state of the cell.
11.2
Battery construction
The word battery simply means a group of similar components. In military vocabulary, a "bat-
tery" refers to a cluster of guns. In electricity, a "battery" is a set of voltaic cells designed to
provide greater voltage and/or current than is possible with one cell alone.
The symbol for a cell is very simple, consisting of one long line and one short line, parallel
to each other, with connecting wires:
Cell
The symbol for a battery is nothing more than a couple of cell symbols stacked in series:
Battery
As was stated before, the voltage produced by any particular kind of cell is determined
strictly by the chemistry of that cell type. The size of the cell is irrelevant to its voltage. To
obtain greater voltage than the output of a single cell, multiple cells must be connected in
series. The total voltage of a battery is the sum of all cell voltages. A typical automotive
lead-acid battery has six cells, for a nominal voltage output of 6 x 2.0 or 12.0 volts:
398
CHAPTER 11. BATTERIES AND POWER SYSTEMS
2.0 V
12.0 V
2.0 V
2.0 V
2.0 V
2.0 V
2.0 V
The cells in an automotive battery are contained within the same hard rubber housing,
connected together with thick, lead bars instead of wires. The electrodes and electrolyte solu-
tions for each cell are contained in separate, partitioned sections of the battery case. In large
batteries, the electrodes commonly take the shape of thin metal grids or plates, and are often
referred to as plates instead of electrodes.
For the sake of convenience, battery symbols are usually limited to four lines, alternating
long/short, although the real battery it represents may have many more cells than that. On
occasion, however, you might come across a symbol for a battery with unusually high voltage,
intentionally drawn with extra lines. The lines, of course, are representative of the individual
cell plates:
symbol for a battery with
an unusually high voltage
If the physical size of a cell has no impact on its voltage, then what does it affect? The
answer is resistance, which in turn affects the maximum amount of current that a cell can
provide. Every voltaic cell contains some amount of internal resistance due to the electrodes
and the electrolyte. The larger a cell is constructed, the greater the electrode contact area with
the electrolyte, and thus the less internal resistance it will have.
Although we generally consider a cell or battery in a circuit to be a perfect source of voltage
(absolutely constant), the current through it dictated solely by the external resistance of the
circuit to which it is attached, this is not entirely true in real life. Since every cell or battery
contains some internal resistance, that resistance must affect the current in any given circuit:
10 V
10 A
1 Ω
Eload = 10 V
10 V
0.2 Ω
1 Ω
Eload = 8.333 V
8.333 A
Ideal battery
Real battery
(with internal resistance)
The real battery shown above within the dotted lines has an internal resistance of 0.2 Ω,
11.2.
BATTERY CONSTRUCTION
399
which affects its ability to supply current to the load resistance of 1 Ω. The ideal battery on the
left has no internal resistance, and so our Ohm's Law calculations for current (I=E/R) give us
a perfect value of 10 amps for current with the 1 ohm load and 10 volt supply. The real battery,
with its built-in resistance further impeding the flow of electrons, can only supply 8.333 amps
to the same resistance load.
The ideal battery, in a short circuit with 0 Ωresistance, would be able to supply an infinite
amount of current. The real battery, on the other hand, can only supply 50 amps (10 volts /
0.2 Ω) to a short circuit of 0 Ωresistance, due to its internal resistance. The chemical reaction
inside the cell may still be providing exactly 10 volts, but voltage is dropped across that internal
resistance as electrons flow through the battery, which reduces the amount of voltage available
at the battery terminals to the load.
Since we live in an imperfect world, with imperfect batteries, we need to understand the
implications of factors such as internal resistance. Typically, batteries are placed in applica-
tions where their internal resistance is negligible compared to that of the circuit load (where
their short-circuit current far exceeds their usual load current), and so the performance is very
close to that of an ideal voltage source.
If we need to construct a battery with lower resistance than what one cell can provide (for
greater current capacity), we will have to connect the cells together in parallel:
equivalent to
0.2 Ω
2.0 V
0.2 Ω
0.2 Ω
0.2 Ω
0.2 Ω
0.04 Ω
2.0 V
2.0 V
2.0 V
2.0 V
2.0 V
Essentially, what we have done here is determine the Thevenin equivalent of the five cells
in parallel (an equivalent network of one voltage source and one series resistance). The equiv-
alent network has the same source voltage but a fraction of the resistance of any individual
cell in the original network. The overall effect of connecting cells in parallel is to decrease the
equivalent internal resistance, just as resistors in parallel diminish in total resistance. The
equivalent internal resistance of this battery of 5 cells is 1/5 that of each individual cell. The
overall voltage stays the same: 2.0 volts. If this battery of cells were powering a circuit, the
current through each cell would be 1/5 of the total circuit current, due to the equal split of
current through equal-resistance parallel branches.
• REVIEW:
• A battery is a cluster of cells connected together for greater voltage and/or current capac-
ity.
• Cells connected together in series (polarities aiding) results in greater total voltage.
• Physical cell size impacts cell resistance, which in turn impacts the ability for the cell to
supply current to a circuit. Generally, the larger the cell, the less its internal resistance.
400
CHAPTER 11. BATTERIES AND POWER SYSTEMS
• Cells connected together in parallel results in less total resistance, and potentially greater
total current.
11.3
Battery ratings
Because batteries create electron flow in a circuit by exchanging electrons in ionic chemical
reactions, and there is a limited number of molecules in any charged battery available to react,
there must be a limited amount of total electrons that any battery can motivate through a
circuit before its energy reserves are exhausted. Battery capacity could be measured in terms
of total number of electrons, but this would be a huge number. We could use the unit of the
coulomb (equal to 6.25 x 1018 electrons, or 6,250,000,000,000,000,000 electrons) to make the
quantities more practical to work with, but instead a new unit, the amp-hour, was made for
this purpose. Since 1 amp is actually a flow rate of 1 coulomb of electrons per second, and
there are 3600 seconds in an hour, we can state a direct proportion between coulombs and
amp-hours: 1 amp-hour = 3600 coulombs. Why make up a new unit when an old would have
done just fine? To make your lives as students and technicians more difficult, of course!
A battery with a capacity of 1 amp-hour should be able to continuously supply a current of
1 amp to a load for exactly 1 hour, or 2 amps for 1/2 hour, or 1/3 amp for 3 hours, etc., before
becoming completely discharged. In an ideal battery, this relationship between continuous
current and discharge time is stable and absolute, but real batteries don't behave exactly as
this simple linear formula would indicate. Therefore, when amp-hour capacity is given for a
battery, it is specified at either a given current, given time, or assumed to be rated for a time
period of 8 hours (if no limiting factor is given).
For example, an average automotive battery might have a capacity of about 70 amp-hours,
specified at a current of 3.5 amps. This means that the amount of time this battery could
continuously supply a current of 3.5 amps to a load would be 20 hours (70 amp-hours / 3.5
amps). But let's suppose that a lower-resistance load were connected to that battery, drawing
70 amps continuously. Our amp-hour equation tells us that the battery should hold out for
exactly 1 hour (70 amp-hours / 70 amps), but this might not be true in real life. With higher
currents, the battery will dissipate more heat across its internal resistance, which has the
effect of altering the chemical reactions taking place within. Chances are, the battery would
fully discharge some time before the calculated time of 1 hour under this greater load.
Conversely, if a very light load (1 mA) were to be connected to the battery, our equation
would tell us that the battery should provide power for 70,000 hours, or just under 8 years (70
amp-hours / 1 milliamp), but the odds are that much of the chemical energy in a real battery
would have been drained due to other factors (evaporation of electrolyte, deterioration of elec-
trodes, leakage current within battery) long before 8 years had elapsed. Therefore, we must
take the amp-hour relationship as being an ideal approximation of battery life, the amp-hour
rating trusted only near the specified current or timespan given by the manufacturer. Some
manufacturers will provide amp-hour derating factors specifying reductions in total capacity
at different levels of current and/or temperature.
For secondary cells, the amp-hour rating provides a rule for necessary charging time at any
given level of charge current. For example, the 70 amp-hour automotive battery in the previous
example should take 10 hours to charge from a fully-discharged state at a constant charging
current of 7 amps (70 amp-hours / 7 amps).
11.3.
BATTERY RATINGS
401
Approximate amp-hour capacities of some common batteries are given here:
• Typical automotive battery: 70 amp-hours @ 3.5 A (secondary cell)
• D-size carbon-zinc battery: 4.5 amp-hours @ 100 mA (primary cell)
• 9 volt carbon-zinc battery: 400 milliamp-hours @ 8 mA (primary cell)
As a battery discharges, not only does it diminish its internal store of energy, but its in-
ternal resistance also increases (as the electrolyte becomes less and less conductive), and its
open-circuit cell voltage decreases (as the chemicals become more and more dilute). The most
deceptive change that a discharging battery exhibits is increased resistance. The best check
for a battery's condition is a voltage measurement under load, while the battery is supply-
ing a substantial current through a circuit. Otherwise, a simple voltmeter check across the
terminals may falsely indicate a healthy battery (adequate voltage) even though the internal
resistance has increased considerably. What constitutes a "substantial current" is determined
by the battery's design parameters. A voltmeter check revealing too low of a voltage, of course,
would positively indicate a discharged battery:
Fully charged battery:
V
Voltmeter indication:
V
Voltmeter indication:
No load
Under load
Scenario for a fully charged battery
0.1 Ω
13.2 V
13.2 V
0.1 Ω
13.2 V
100 Ω
13.187 V
Now, if the battery discharges a bit . . .
V
Voltmeter indication:
V
Voltmeter indication:
No load
Under load
13.0 V
5 Ω
13.0 V
Scenario for a slightly discharged battery
5 Ω
13.0 V
100 Ω
12.381 V
. . . and discharges a bit further . . .
V
Voltmeter indication:
V
Voltmeter indication:
No load
Under load
20 Ω
11.5 V
20 Ω
11.5 V
11.5 V
100 Ω
9.583 V
Scenario for a moderately discharged battery
402
CHAPTER 11. BATTERIES AND POWER SYSTEMS
. . . and a bit further until its dead.
V
Voltmeter indication:
V
Voltmeter indication:
No load
Under load
50 Ω
7.5 V
50 Ω
7.5 V
7.5 V
100 Ω
5 V
Scenario for a dead battery
Notice how much better the battery's true condition is revealed when its voltage is checked
under load as opposed to without a load. Does this mean that its pointless to check a battery
with just a voltmeter (no load)? Well, no. If a simple voltmeter check reveals only 7.5 volts for
a 13.2 volt battery, then you know without a doubt that its dead. However, if the voltmeter
were to indicate 12.5 volts, it may be near full charge or somewhat depleted - you couldn't tell
without a load check. Bear in mind also that the resistance used to place a battery under load
must be rated for the amount of power expected to be dissipated. For checking large batteries
such as an automobile (12 volt nominal) lead-acid battery, this may mean a resistor with a
power rating of several hundred watts.
• REVIEW:
• The amp-hour is a unit of battery energy capacity, equal to the amount of continuous
current multiplied by the discharge time, that a battery can supply before exhausting its
internal store of chemical energy.
Continuous current (in Amps) =
Amp-hour rating
Charge/discharge time (in hours)
Charge/discharge time (in hours) =
Amp-hour rating
Continuous current (in Amps)
• An amp-hour battery rating is only an approximation of the battery's charge capacity,
and should be trusted only at the current level or time specified by the manufacturer.
Such a rating cannot be extrapolated for very high currents or very long times with any
accuracy.
• Discharged batteries lose voltage and increase in resistance. The best check for a dead
battery is a voltage test under load.
11.4
Special-purpose batteries
Back in the early days of electrical measurement technology, a special type of battery known
as a mercury standard cell was popularly used as a voltage calibration standard. The output
of a mercury cell was 1.0183 to 1.0194 volts DC (depending on the specific design of cell), and
was extremely stable over time. Advertised drift was around 0.004 percent of rated voltage per
year. Mercury standard cells were sometimes known as Weston cells or cadmium cells.
11.4.
SPECIAL-PURPOSE BATTERIES
403
cork washer
cork washer
mercury
cadmium amalgam
mercurous
sulphate
cadmium
sulphate
solution
cadmium
sulphate
solution
wire
wire
glass bulb
Mercury "standard" cell
Hg2SO4
CdSO4
CdSO4
Unfortunately, mercury cells were rather intolerant of any current drain and could not
even be measured with an analog voltmeter without compromising accuracy. Manufacturers
typically called for no more than 0.1 mA of current through the cell, and even that figure
was considered a momentary, or surge maximum! Consequently, standard cells could only
be measured with a potentiometric (null-balance) device where current drain is almost zero.
Short-circuiting a mercury cell was prohibited, and once short-circuited, the cell could never
be relied upon again as a standard device.
Mercury standard cells were also susceptible to slight changes in voltage if physically or
thermally disturbed. Two different types of mercury standard cells were developed for differ-
ent calibration purposes: saturated and unsaturated. Saturated standard cells provided the
greatest voltage stability over time, at the expense of thermal instability. In other words, their
voltage drifted very little with the passage of time (just a few microvolts over the span of a
decade!), but tended to vary with changes in temperature (tens of microvolts per degree Cel-
sius). These cells functioned best in temperature-controlled laboratory environments where
long-term stability is paramount. Unsaturated cells provided thermal stability at the expense
of stability over time, the voltage remaining virtually constant with changes in temperature
but decreasing steadily by about 100 μV every year. These cells functioned best as "field" cali-
bration devices where ambient temperature is not precisely controlled. Nominal voltage for a
saturated cell was 1.0186 volts, and 1.019 volts for an unsaturated cell.
Modern semiconductor voltage (zener diode regulator) references have superseded standard
cell batteries as laboratory and field voltage standards.
A fascinating device closely related to primary-cell batteries is the fuel cell, so-called be-
cause it harnesses the chemical reaction of combustion to generate an electric current. The
process of chemical oxidation (oxygen ionically bonding with other elements) is capable of pro-
ducing an electron flow between two electrodes just as well as any combination of metals and
electrolytes. A fuel cell can be thought of as a battery with an externally supplied chemical
404
CHAPTER 11. BATTERIES AND POWER SYSTEMS
energy source.
water out
oxygen in
hydrogen in
electrolyte
membranes
electrodes
load
H2
H2
H2
H2
H2
H2
O2
O2
O2
O2
O2
O2
Hydrogen/Oxygen fuel cell
To date, the most successful fuel cells constructed are those which run on hydrogen and oxy-
gen, although much research has been done on cells using hydrocarbon fuels. While "burning"
hydrogen, a fuel cell's only waste byproducts are water and a small amount of heat. When op-
erating on carbon-containing fuels, carbon dioxide is also released as a byproduct. Because the
operating temperature of modern fuel cells is far below that of normal combustion, no oxides
of nitrogen (NOx) are formed, making it far less polluting, all other factors being equal.
The efficiency of energy conversion in a fuel cell from chemical to electrical far exceeds
the theoretical Carnot efficiency limit of any internal-combustion engine, which is an exciting
prospect for power generation and hybrid electric automobiles.
Another type of "battery" is the solar cell, a by-product of the semiconductor revolution in
electronics. The photoelectric effect, whereby electrons are dislodged from atoms under the
influence of light, has been known in physics for many decades, but it has only been with
recent advances in semiconductor technology that a device existed capable of harnessing this
effect to any practical degree. Conversion efficiencies for silicon solar cells are still quite low,
but their benefits as power sources are legion: no moving parts, no noise, no waste products or
pollution (aside from the manufacture of solar cells, which is still a fairly "dirty" industry), and
indefinite life.
11.4.
SPECIAL-PURPOSE BATTERIES
405
thin, round wafer of
crystalline silicon
wires
schematic symbol
Solar cell
Specific cost of solar cell technology (dollars per kilowatt) is still very high, with little
prospect of significant decrease barring some kind of revolutionary advance in technology. Un-
like electronic components made from semiconductor material, which can be made smaller and
smaller with less scrap as a result of better quality control, a single solar cell still takes the
same amount of ultra-pure silicon to make as it did thirty years ago. Superior quality control
fails to yield the same production gain seen in the manufacture of chips and transistors (where
isolated specks of impurity can ruin many microscopic circuits on one wafer of silicon). The
same number of impure inclusions does little to impact the overall efficiency of a 3-inch solar
cell.
Yet another type of special-purpose "battery" is the chemical detection cell. Simply put,
these cells chemically react with specific substances in the air to create a voltage directly pro-
portional to the concentration of that substance. A common application for a chemical detection
cell is in the detection and measurement of oxygen concentration. Many portable oxygen ana-
lyzers have been designed around these small cells. Cell chemistry must be designed to match
the specific substance(s) to be detected, and the cells do tend to "wear out," as their electrode
materials deplete or become contaminated with use.
• REVIEW:
• mercury standard cells are special types of batteries which were once used as voltage
calibration standards before the advent of precision semiconductor reference devices.
• A fuel cell is a kind of battery that uses a combustible fuel and oxidizer as reactants to
generate electricity. They are promising sources of electrical power in the future, "burn-
ing" fuels with very low emissions.
• A solar cell uses ambient light energy to motivate electrons from one electrode to the
other, producing voltage (and current, providing an external circuit).
• A chemical detection cell is a special type of voltaic cell which produces voltage propor-
tional to the concentration of an applied substance (usually a specific gas in ambient air).
406
CHAPTER 11. BATTERIES AND POWER SYSTEMS
11.5
Practical considerations
When connecting batteries together to form larger "banks" (a battery of batteries?), the con-
stituent batteries must be matched to each other so as to not cause problems. First we will
consider connecting batteries in series for greater voltage:
load
We know that the current is equal at all points in a series circuit, so whatever amount of
current there is in any one of the series-connected batteries must be the same for all the others
as well. For this reason, each battery must have the same amp-hour rating, or else some of the
batteries will become depleted sooner than others, compromising the capacity of the whole bank.
Please note that the total amp-hour capacity of this series battery bank is not affected by the
number of batteries.
Next, we will consider connecting batteries in parallel for greater current capacity (lower
internal resistance), or greater amp-hour capacity:
load
We know that the voltage is equal across all branches of a parallel circuit, so we must be
sure that these batteries are of equal voltage. If not, we will have relatively large currents
circulating from one battery through another, the higher-voltage batteries overpowering the
lower-voltage batteries. This is not good.
On this same theme, we must be sure that any overcurrent protection (circuit breakers or
fuses) are installed in such a way as to be effective. For our series battery bank, one fuse will
suffice to protect the wiring from excessive current, since any break in a series circuit stops
current through all parts of the circuit:
load
fuse
With a parallel battery bank, one fuse is adequate for protecting the wiring against load
overcurrent (between the parallel-connected batteries and the load), but we have other con-
11.5.
PRACTICAL CONSIDERATIONS
407
cerns to protect against as well. Batteries have been known to internally short-circuit, due
to electrode separator failure, causing a problem not unlike that where batteries of unequal
voltage are connected in parallel: the good batteries will overpower the failed (lower voltage)
battery, causing relatively large currents within the batteries' connecting wires.
To guard
against this eventuality, we should protect each and every battery against overcurrent with
individual battery fuses, in addition to the load fuse:
load
main
fuse
When dealing with secondary-cell batteries, particular attention must be paid to the method
and timing of charging. Different types and construction of batteries have different charging
needs, and the manufacturer's recommendations are probably the best guide to follow when
designing or maintaining a system. Two distinct concerns of battery charging are cycling and
overcharging.
Cycling refers to the process of charging a battery to a "full" condition and
then discharging it to a lower state. All batteries have a finite (limited) cycle life, and the
allowable "depth" of cycle (how far it should be discharged at any time) varies from design to
design. Overcharging is the condition where current continues to be forced backwards through
a secondary cell beyond the point where the cell has reached full charge. With lead-acid cells
in particular, overcharging leads to electrolysis of the water ("boiling" the water out of the
battery) and shortened life.
Any battery containing water in the electrolyte is subject to the production of hydrogen gas
due to electrolysis. This is especially true for overcharged lead-acid cells, but not exclusive to
that type. Hydrogen is an extremely flammable gas (especially in the presence of free oxygen
created by the same electrolysis process), odorless and colorless. Such batteries pose an explo-
sion threat even under normal operating conditions, and must be treated with respect. The
author has been a firsthand witness to a lead-acid battery explosion, where a spark created by
the removal of a battery charger (small DC power supply) from an automotive battery ignited
hydrogen gas within the battery case, blowing the top off the battery and splashing sulfuric
acid everywhere. This occurred in a high school automotive shop, no less. If it were not for
all the students nearby wearing safety glasses and buttoned-collar overalls, significant injury
could have occurred.
When connecting and disconnecting charging equipment to a battery, always make the last
connection (or first disconnection) at a location away from the battery itself (such as at a point
on one of the battery cables, at least a foot away from the battery), so that any resultant spark
has little or no chance of igniting hydrogen gas.
In large, permanently installed battery banks, batteries are equipped with vent caps above
each cell, and hydrogen gas is vented outside of the battery room through hoods immediately
over the batteries. Hydrogen gas is very light and rises quickly. The greatest danger is when
it is allowed to accumulate in an area, awaiting ignition.
408
CHAPTER 11. BATTERIES AND POWER SYSTEMS
More modern lead-acid battery designs are sealed, fabricated to re-combine the electrolyzed
hydrogen and oxygen back into water, inside the battery case itself. Adequate ventilation might
still be a good idea, just in case a battery were to develop a leak. [2]
• REVIEW:
• Connecting batteries in series increases voltage, but does not increase overall amp-hour
capacity.
• All batteries in a series bank must have the same amp-hour rating.
• Connecting batteries in parallel increases total current capacity by decreasing total re-
sistance, and it also increases overall amp-hour capacity.
• All batteries in a parallel bank must have the same voltage rating.
• Batteries can be damaged by excessive cycling and overcharging.
• Water-based electrolyte batteries are capable of generating explosive hydrogen gas, which
must not be allowed to accumulate in an area.
11.6
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
John Anhalt (December 2008): Updated Lead-acid cell chemistry..
Bibliography
Handbook,
Primer
on
Lead-Acid
Storage
ies",
September
1995,
pp.
13.
at
http://www.hss.energy.gov/NuclearSafety/techstds/standard/
hdbk1084/hdbk1084.pdf
Nelson,
"The
Basic
Chemistry
of
Gas
Recombination
in
Lead-Acid
Batteries",
JOM,
53
(1)
(2001),
pp.
at
Chapter 12
PHYSICS OF CONDUCTORS
AND INSULATORS
Contents
12.1 Introduction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 409
12.2 Conductor size
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 411
12.3 Conductor ampacity . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 417
12.4 Fuses . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 419
12.5 Specific resistance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 426
12.6 Temperature coefficient of resistance . . . . . . . . . . . . . . . . . . . . . . 431
12.7 Superconductivity . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 434
12.8 Insulator breakdown voltage
. . . . . . . . . . . . . . . . . . . . . . . . . . . 436
12.9 Data
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 438
12.10Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 438
12.1
Introduction
By now you should be well aware of the correlation between electrical conductivity and cer-
tain types of materials. Those materials allowing for easy passage of free electrons are called
conductors, while those materials impeding the passage of free electrons are called insulators.
Unfortunately, the scientific theories explaining why certain materials conduct and others
don't are quite complex, rooted in quantum mechanical explanations in how electrons are ar-
ranged around the nuclei of atoms. Contrary to the well-known "planetary" model of electrons
whirling around an atom's nucleus as well-defined chunks of matter in circular or elliptical
orbits, electrons in "orbit" don't really act like pieces of matter at all. Rather, they exhibit
the characteristics of both particle and wave, their behavior constrained by placement within
distinct zones around the nucleus referred to as "shells" and "subshells." Electrons can occupy
409
410
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
these zones only in a limited range of energies depending on the particular zone and how oc-
cupied that zone is with other electrons. If electrons really did act like tiny planets held in
orbit around the nucleus by electrostatic attraction, their actions described by the same laws
describing the motions of real planets, there could be no real distinction between conductors
and insulators, and chemical bonds between atoms would not exist in the way they do now. It
is the discrete, "quantitized" nature of electron energy and placement described by quantum
physics that gives these phenomena their regularity.
When an electron is free to assume higher energy states around an atom's nucleus (due to
its placement in a particular "shell"), it may be free to break away from the atom and comprise
part of an electric current through the substance. If the quantum limitations imposed on an
electron deny it this freedom, however, the electron is considered to be "bound" and cannot
break away (at least not easily) to constitute a current.
The former scenario is typical of
conducting materials, while the latter is typical of insulating materials.
Some textbooks will tell you that an element's conductivity or nonconductivity is exclusively
determined by the number of electrons residing in the atoms' outer "shell" (called the valence
shell), but this is an oversimplification, as any examination of conductivity versus valence
electrons in a table of elements will confirm. The true complexity of the situation is further
revealed when the conductivity of molecules (collections of atoms bound to one another by
electron activity) is considered.
A good example of this is the element carbon, which comprises materials of vastly differing
conductivity: graphite and diamond. Graphite is a fair conductor of electricity, while diamond
is practically an insulator (stranger yet, it is technically classified as a semiconductor, which
in its pure form acts as an insulator, but can conduct under high temperatures and/or the
influence of impurities). Both graphite and diamond are composed of the exact same types of
atoms: carbon, with 6 protons, 6 neutrons and 6 electrons each. The fundamental difference
between graphite and diamond being that graphite molecules are flat groupings of carbon
atoms while diamond molecules are tetrahedral (pyramid-shaped) groupings of carbon atoms.
If atoms of carbon are joined to other types of atoms to form compounds, electrical conduc-
tivity becomes altered once again. Silicon carbide, a compound of the elements silicon and car-
bon, exhibits nonlinear behavior: its electrical resistance decreases with increases in applied
voltage! Hydrocarbon compounds (such as the molecules found in oils) tend to be very good
insulators. As you can see, a simple count of valence electrons in an atom is a poor indicator of
a substance's electrical conductivity.
All metallic elements are good conductors of electricity, due to the way the atoms bond with
each other. The electrons of the atoms comprising a mass of metal are so uninhibited in their
allowable energy states that they float freely between the different nuclei in the substance,
readily motivated by any electric field.
The electrons are so mobile, in fact, that they are
sometimes described by scientists as an electron gas, or even an electron sea in which the
atomic nuclei rest. This electron mobility accounts for some of the other common properties of
metals: good heat conductivity, malleability and ductility (easily formed into different shapes),
and a lustrous finish when pure.
Thankfully, the physics behind all this is mostly irrelevant to our purposes here. Suffice it
to say that some materials are good conductors, some are poor conductors, and some are in be-
tween. For now it is good enough to simply understand that these distinctions are determined
by the configuration of the electrons around the constituent atoms of the material.
An important step in getting electricity to do our bidding is to be able to construct paths
12.2.
CONDUCTOR SIZE
411
for electrons to flow with controlled amounts of resistance. It is also vitally important that we
be able to prevent electrons from flowing where we don't want them to, by using insulating
materials. However, not all conductors are the same, and neither are all insulators. We need
to understand some of the characteristics of common conductors and insulators, and be able to
apply these characteristics to specific applications.
Almost all conductors possess a certain, measurable resistance (special types of materials
called superconductors possess absolutely no electrical resistance, but these are not ordinary
materials, and they must be held in special conditions in order to be super conductive). Typ-
ically, we assume the resistance of the conductors in a circuit to be zero, and we expect that
current passes through them without producing any appreciable voltage drop. In reality, how-
ever, there will almost always be a voltage drop along the (normal) conductive pathways of an
electric circuit, whether we want a voltage drop to be there or not:
Source
Load
wire resistance
wire resistance
voltage
drop
something less than
source voltage
drop
voltage
In order to calculate what these voltage drops will be in any particular circuit, we must be
able to ascertain the resistance of ordinary wire, knowing the wire size and diameter. Some of
the following sections of this chapter will address the details of doing this.
• REVIEW:
• Electrical conductivity of a material is determined by the configuration of electrons in
that materials atoms and molecules (groups of bonded atoms).
• All normal conductors possess resistance to some degree.
• Electrons flowing through a conductor with (any) resistance will produce some amount of
voltage drop across the length of that conductor.
12.2
Conductor size
It should be common-sense knowledge that liquids flow through large-diameter pipes easier
than they do through small-diameter pipes (if you would like a practical illustration, try drink-
ing a liquid through straws of different diameters). The same general principle holds for the
flow of electrons through conductors: the broader the cross-sectional area (thickness) of the
conductor, the more room for electrons to flow, and consequently, the easier it is for flow to
occur (less resistance).
Electrical wire is usually round in cross-section (although there are some unique exceptions
to this rule), and comes in two basic varieties: solid and stranded. Solid copper wire is just as it
412
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
sounds: a single, solid strand of copper the whole length of the wire. Stranded wire is composed
of smaller strands of solid copper wire twisted together to form a single, larger conductor. The
greatest benefit of stranded wire is its mechanical flexibility, being able to withstand repeated
bending and twisting much better than solid copper (which tends to fatigue and break after
time).
Wire size can be measured in several ways. We could speak of a wire's diameter, but since
its really the cross-sectional area that matters most regarding the flow of electrons, we are
better off designating wire size in terms of area.
end-view of
solid round wire
0.1019
inches
Cross-sectional area
is 0.008155 square inches
The wire cross-section picture shown above is, of course, not drawn to scale. The diameter
is shown as being 0.1019 inches. Calculating the area of the cross-section with the formula
Area = πr2, we get an area of 0.008155 square inches:
0.1019
2
2
A = 0.008155 square inches
inches
These are fairly small numbers to work with, so wire sizes are often expressed in measures
of thousandths-of-an-inch, or mils. For the illustrated example, we would say that the diameter
of the wire was 101.9 mils (0.1019 inch times 1000). We could also, if we wanted, express the
area of the wire in the unit of square mils, calculating that value with the same circle-area
formula, Area = πr2:
12.2.
CONDUCTOR SIZE
413
end-view of
solid round wire
Cross-sectional area
101.9
mils
is 8155.27 square mils
2
2
101.9
A = 8155.27 square mils
mils
However, electricians and others frequently concerned with wire size use another unit of
area measurement tailored specifically for wire's circular cross-section. This special unit is
called the circular mil (sometimes abbreviated cmil). The sole purpose for having this special
unit of measurement is to eliminate the need to invoke the factor π (3.1415927 .
in
the formula for calculating area, plus the need to figure wire radius when you've been given
diameter. The formula for calculating the circular-mil area of a circular wire is very simple:
Circular Wire Area Formula
Because this is a unit of area measurement, the mathematical power of 2 is still in effect
(doubling the width of a circle will always quadruple its area, no matter what units are used,
or if the width of that circle is expressed in terms of radius or diameter). To illustrate the
difference between measurements in square mils and measurements in circular mils, I will
compare a circle with a square, showing the area of each shape in both unit measures:
1 mil
1 mil
Area = 0.7854 square mils
Area = 1 circular mil
Area = 1 square mil
Area = 1.273 circular mils
414
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
And for another size of wire:
2 mils
Area = 4 circular mils
Area = 3.1416 square mils
2 mils
Area = 5.0930 circular mils
Area = 4 square mils
Obviously, the circle of a given diameter has less cross-sectional area than a square of
width and height equal to the circle's diameter: both units of area measurement reflect that.
However, it should be clear that the unit of "square mil" is really tailored for the convenient
determination of a square's area, while "circular mil" is tailored for the convenient determi-
nation of a circle's area: the respective formula for each is simpler to work with. It must be
understood that both units are valid for measuring the area of a shape, no matter what shape
that may be. The conversion between circular mils and square mils is a simple ratio: there are
π (3.1415927 . . .) square mils to every 4 circular mils.
Another measure of cross-sectional wire area is the gauge. The gauge scale is based on
whole numbers rather than fractional or decimal inches. The larger the gauge number, the
skinnier the wire; the smaller the gauge number, the fatter the wire. For those acquainted
with shotguns, this inversely-proportional measurement scale should sound familiar.
The table at the end of this section equates gauge with inch diameter, circular mils, and
square inches for solid wire. The larger sizes of wire reach an end of the common gauge scale
(which naturally tops out at a value of 1), and are represented by a series of zeros.
is another way to represent "000," and is pronounced "triple-ought." Again, those acquainted
with shotguns should recognize the terminology, strange as it may sound. To make matters
even more confusing, there is more than one gauge "standard" in use around the world. For
electrical conductor sizing, the American Wire Gauge (AWG), also known as the Brown and
Sharpe (B&S) gauge, is the measurement system of choice. In Canada and Great Britain, the
British Standard Wire Gauge (SWG) is the legal measurement system for electrical conductors.
Other wire gauge systems exist in the world for classifying wire diameter, such as the Stubs
steel wire gauge and the Steel Music Wire Gauge (MWG), but these measurement systems
apply to non-electrical wire use.
The American Wire Gauge (AWG) measurement system, despite its oddities, was designed
with a purpose: for every three steps in the gauge scale, wire area (and weight per unit length)
approximately doubles. This is a handy rule to remember when making rough wire size esti-
mations!
For very large wire sizes (fatter than 4/0), the wire gauge system is typically abandoned
for cross-sectional area measurement in thousands of circular mils (MCM), borrowing the old
12.2.
CONDUCTOR SIZE
415
Roman numeral "M" to denote a multiple of "thousand" in front of "CM" for "circular mils." The
following table of wire sizes does not show any sizes bigger than 4/0 gauge, because solid cop-
per wire becomes impractical to handle at those sizes. Stranded wire construction is favored,
instead.
Soild copper wire table: 12.1
For some high-current applications, conductor sizes beyond the practical size limit of round
wire are required. In these instances, thick bars of solid metal called busbars are used as
conductors. Busbars are usually made of copper or aluminum, and are most often uninsulated.
They are physically supported away from whatever framework or structure is holding them
by insulator standoff mounts. Although a square or rectangular cross-section is very common
for busbar shape, other shapes are used as well. Cross-sectional area for busbars is typically
rated in terms of circular mils (even for square and rectangular bars!), most likely for the
convenience of being able to directly equate busbar size with round wire.
• REVIEW:
• Electrons flow through large-diameter wires easier than small-diameter wires, due to the
greater cross-sectional area they have in which to move.
• Rather than measure small wire sizes in inches, the unit of "mil" (1/1000 of an inch) is
often employed.
• The cross-sectional area of a wire can be expressed in terms of square units (square inches
or square mils), circular mils, or "gauge" scale.
• Calculating square-unit wire area for a circular wire involves the circle area formula:
(Square units)
• Calculating circular-mil wire area for a circular wire is much simpler, due to the fact that
the unit of "circular mil" was sized just for this purpose: to eliminate the "pi" and the d/2
(radius) factors in the formula.
(Circular units)
• There are π (3.1416) square mils for every 4 circular mils.
• The gauge system of wire sizing is based on whole numbers, larger numbers representing
smaller-area wires and vice versa. Wires thicker than 1 gauge are represented by zeros: 0,
00, 000, and 0000 (spoken "single-ought," "double-ought," "triple-ought," and "quadruple-
ought."
• Very large wire sizes are rated in thousands of circular mils (MCM's), typical for busbars
and wire sizes beyond 4/0.
• Busbars are solid bars of copper or aluminum used in high-current circuit construction.
Connections made to busbars are usually welded or bolted, and the busbars are often bare
(uninsulated), supported away from metal frames through the use of insulating standoffs.
416
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
Table 12.1: Soild copper wire table:
Size
Diameter
Cross-sectional
area
Weight
AWG
inches
cir. mils
sq. inches
lb/1000 ft
0.4600
211,600
0.1662
640.5
0.4096
167,800
0.1318
507.9
0.3648
133,100
0.1045
402.8
0.3249
105,500
0.08289
319.5
1
0.2893
83,690
0.06573
253.5
2
0.2576
66,370
0.05213
200.9
3
0.2294
52,630
0.04134
159.3
4
0.2043
41,740
0.03278
126.4
5
0.1819
33,100
0.02600
100.2
6
0.1620
26,250
0.02062
79.46
7
0.1443
20,820
0.01635
63.02
7
0.1443
20,820
0.01635
63.02
8
0.1285
16,510
0.01297
49.97
9
0.1144
13,090
0.01028
39.63
10
0.1019
10,380
0.008155
31.43
11
0.09074
8,234
0.006467
24.92
12
0.08081
6,530
0.005129
19.77
13
0.07196
5,178
0.004067
15.68
14
0.06408
4,107
0.003225
12.43
15
0.05707
3,257
0.002558
9.858
16
0.05082
2,583
0.002028
7.818
17
0.04526
2,048
0.001609
6.200
18
0.04030
1,624
0.001276
4.917
19
0.03589
1,288
0.001012
3.899
20
0.03196
1,022
0.0008023
3.092
21
0.02846
810.1
0.0006363
2.452
22
0.02535
642.5
0.0005046
1.945
23
0.02257
509.5
0.0004001
1.542
23
0.02257
509.5
0.0004001
1.542
24
0.02010
404.0
0.0003173
1.233
25
0.01790
320.4
0.0002517
0.9699
26
0.01594
254.1
0.0001996
0.7692
27
0.01420
201.5
0.0001583
0.6100
28
0.01264
159.8
0.0001255
0.4837
29
0.01126
126.7
0.00009954
0.3836
30
0.01003
100.5
0.00007894
0.3042
31
0.008928
79.70
0.00006260
0.2413
32
0.007950
63.21
0.00004964
0.1913
33
0.007080
50.13
0.00003937
0.1517
34
0.006305
39.75
0.00003122
0.1203
35
0.005615
31.52
0.00002476
0.09542
36
0.005000
25.00
0.00001963
0.07567
37
0.004453
19.83
0.00001557
0.06001
38
0.003965
15.72
0.00001235
0.04759
39
0.003531
12.47
0.000009793
0.03774
40
0.003145
9.888
0.000007766
0.02993
41
0.002800
7.842
0.000006159
0.02374
42
0.002494
6.219
0.000004884
0.01882
43
0.002221
4.932
0.000003873
0.01493
44
0.001978
3.911
0.000003072
0.01184
12.3.
CONDUCTOR AMPACITY
417
12.3
Conductor ampacity
The smaller the wire, the greater the resistance for any given length, all other factors being
equal. A wire with greater resistance will dissipate a greater amount of heat energy for any
given amount of current, the power being equal to P=I2R.
Dissipated power in a resistance manifests itself in the form of heat, and excessive heat
can be damaging to a wire (not to mention objects near the wire!), especially considering the
fact that most wires are insulated with a plastic or rubber coating, which can melt and burn.
Thin wires will, therefore, tolerate less current than thick wires, all other factors being equal.
A conductor's current-carrying limit is known as its ampacity.
Primarily for reasons of safety, certain standards for electrical wiring have been established
within the United States, and are specified in the National Electrical Code (NEC). Typical
NEC wire ampacity tables will show allowable maximum currents for different sizes and ap-
plications of wire. Though the melting point of copper theoretically imposes a limit on wire
ampacity, the materials commonly employed for insulating conductors melt at temperatures
far below the melting point of copper, and so practical ampacity ratings are based on the ther-
mal limits of the insulation. Voltage dropped as a result of excessive wire resistance is also
a factor in sizing conductors for their use in circuits, but this consideration is better assessed
through more complex means (which we will cover in this chapter). A table derived from an
NEC listing is shown for example:
Ampacities of copper wire: 12.2
Table 12.2: Ampacities of copper wire, in free air at 30o C:
INSULATION TYPE:
RUW, T
THW, THWN
FEP, FEPB
TW
RUH
THHN, XHHW
Size
Current Rating
Current Rating
Current Rating
AWG
@ 60 degrees C
@ 75 degrees C
@ 90 degrees C
20
18
18
16
24
14
25
30
35
12
30
35
40
10
40
50
55
8
60
70
80
6
80
95
105
4
105
125
140
2
140
170
190
1
165
195
220
195
230
260
225
265
300
260
310
350
300
360
405
418
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
* = estimated values; normally, these small wire sizes are not manufactured with these
insulation types, 12.2.
Notice the substantial ampacity differences between same-size wires with different types
of insulation. This is due, again, to the thermal limits (60o, 75o, 90o) of each type of insulation
material.
These ampacity ratings are given for copper conductors in "free air" (maximum typical
air circulation), as opposed to wires placed in conduit or wire trays. As you will notice, the
table fails to specify ampacities for small wire sizes. This is because the NEC concerns itself
primarily with power wiring (large currents, big wires) rather than with wires common to
low-current electronic work.
There is meaning in the letter sequences used to identify conductor types, and these letters
usually refer to properties of the conductor's insulating layer(s). Some of these letters symbol-
ize individual properties of the wire while others are simply abbreviations. For example, the
letter "T" by itself means "thermoplastic" as an insulation material, as in "TW" or "THHN."
However, the three-letter combination "MTW" is an abbreviation for Machine Tool Wire, a type
of wire whose insulation is made to be flexible for use in machines experiencing significant
motion or vibration.
Wire insulation codes: 12.3
Table 12.3: Soild copper wire table:
Code
Insulation Material
C
Cotton
FEP
Fluorinated Ethylene Propylene
MI
Mineral (magnesium oxide)
PFA
Perfluoroalkoxy
R
Rubber (sometimes Neoprene)
S
Silicone "rubber"
SA
Silicone-asbestos
T
Thermoplastic
TA
Thermoplastic-asbestos
TFE
Polytetrafluoroethylene ("Teflon")
X
Cross-linked synthetic polymer
Z
Modified ethylene tetrafluoroethylene
Heat rating
H
75 degrees Celsius
HH
90 degrees Celsius
Outer covering ("jacket")
N
Nylon
Special service conditions
U
Underground
W
Wet
90 degrees Celsius and wet
12.4.
FUSES
419
Therefore, a "THWN" conductor has Thermoplastic insulation, is Heat resistant to 75o
Celsius, is rated for Wet conditions, and comes with a Nylon outer jacketing.
Letter codes like these are only used for general-purpose wires such as those used in house-
holds and businesses. For high-power applications and/or severe service conditions, the com-
plexity of conductor technology defies classification according to a few letter codes. Overhead
power line conductors are typically bare metal, suspended from towers by glass, porcelain, or
ceramic mounts known as insulators. Even so, the actual construction of the wire to with-
stand physical forces both static (dead weight) and dynamic (wind) loading can be complex,
with multiple layers and different types of metals wound together to form a single conductor.
Large, underground power conductors are sometimes insulated by paper, then enclosed in a
steel pipe filled with pressurized nitrogen or oil to prevent water intrusion. Such conductors
require support equipment to maintain fluid pressure throughout the pipe.
Other insulating materials find use in small-scale applications. For instance, the small-
diameter wire used to make electromagnets (coils producing a magnetic field from the flow of
electrons) are often insulated with a thin layer of enamel. The enamel is an excellent insulating
material and is very thin, allowing many "turns" of wire to be wound in a small space.
• REVIEW:
• Wire resistance creates heat in operating circuits. This heat is a potential fire ignition
hazard.
• Skinny wires have a lower allowable current ("ampacity") than fat wires, due to their
greater resistance per unit length, and consequently greater heat generation per unit
current.
• The National Electrical Code (NEC) specifies ampacities for power wiring based on al-
lowable insulation temperature and wire application.
12.4
Fuses
Normally, the ampacity rating of a conductor is a circuit design limit never to be intentionally
exceeded, but there is an application where ampacity exceedence is expected: in the case of
fuses.
A fuse is nothing more than a short length of wire designed to melt and separate in the
event of excessive current. Fuses are always connected in series with the component(s) to be
protected from overcurrent, so that when the fuse blows (opens) it will open the entire circuit
and stop current through the component(s). A fuse connected in one branch of a parallel circuit,
of course, would not affect current through any of the other branches.
Normally, the thin piece of fuse wire is contained within a safety sheath to minimize haz-
ards of arc blast if the wire burns open with violent force, as can happen in the case of severe
overcurrents. In the case of small automotive fuses, the sheath is transparent so that the
fusible element can be visually inspected. Residential wiring used to commonly employ screw-
in fuses with glass bodies and a thin, narrow metal foil strip in the middle. A photograph
showing both types of fuses is shown here:
420
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
Cartridge type fuses are popular in automotive applications, and in industrial applications
when constructed with sheath materials other than glass. Because fuses are designed to "fail"
open when their current rating is exceeded, they are typically designed to be replaced easily in
a circuit. This means they will be inserted into some type of holder rather than being directly
soldered or bolted to the circuit conductors. The following is a photograph showing a couple of
glass cartridge fuses in a multi-fuse holder:
12.4.
FUSES
421
The fuses are held by spring metal clips, the clips themselves being permanently connected
to the circuit conductors. The base material of the fuse holder (or fuse block as they are some-
times called) is chosen to be a good insulator.
Another type of fuse holder for cartridge-type fuses is commonly used for installation in
equipment control panels, where it is desirable to conceal all electrical contact points from
human contact. Unlike the fuse block just shown, where all the metal clips are openly exposed,
this type of fuse holder completely encloses the fuse in an insulating housing:
422
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
The most common device in use for overcurrent protection in high-current circuits today
is the circuit breaker.
Circuit breakers are specially designed switches that automatically
open to stop current in the event of an overcurrent condition. Small circuit breakers, such
as those used in residential, commercial and light industrial service are thermally operated.
They contain a bimetallic strip (a thin strip of two metals bonded back-to-back) carrying circuit
current, which bends when heated. When enough force is generated by the bimetallic strip
(due to overcurrent heating of the strip), the trip mechanism is actuated and the breaker will
open. Larger circuit breakers are automatically actuated by the strength of the magnetic field
produced by current-carrying conductors within the breaker, or can be triggered to trip by
external devices monitoring the circuit current (those devices being called protective relays).
Because circuit breakers don't fail when subjected to overcurrent conditions - rather, they
merely open and can be re-closed by moving a lever - they are more likely to be found connected
to a circuit in a more permanent manner than fuses. A photograph of a small circuit breaker
is shown here:
12.4.
FUSES
423
From outside appearances, it looks like nothing more than a switch. Indeed, it could be
used as such. However, its true function is to operate as an overcurrent protection device.
It should be noted that some automobiles use inexpensive devices known as fusible links for
overcurrent protection in the battery charging circuit, due to the expense of a properly-rated
fuse and holder. A fusible link is a primitive fuse, being nothing more than a short piece of
rubber-insulated wire designed to melt open in the event of overcurrent, with no hard sheath-
ing of any kind. Such crude and potentially dangerous devices are never used in industry or
even residential power use, mainly due to the greater voltage and current levels encountered.
As far as this author is concerned, their application even in automotive circuits is questionable.
The electrical schematic drawing symbol for a fuse is an S-shaped curve:
Fuse
Fuses are primarily rated, as one might expect, in the unit for current: amps. Although
their operation depends on the self-generation of heat under conditions of excessive current
by means of the fuse's own electrical resistance, they are engineered to contribute a negligible
amount of extra resistance to the circuits they protect. This is largely accomplished by making
the fuse wire as short as is practically possible. Just as a normal wire's ampacity is not related
to its length (10-gauge solid copper wire will handle 40 amps of current in free air, regardless
of how long or short of a piece it is), a fuse wire of certain material and gauge will blow at
a certain current no matter how long it is. Since length is not a factor in current rating, the
424
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
shorter it can be made, the less resistance it will have end-to-end.
However, the fuse designer also has to consider what happens after a fuse blows: the melted
ends of the once-continuous wire will be separated by an air gap, with full supply voltage
between the ends. If the fuse isn't made long enough on a high-voltage circuit, a spark may be
able to jump from one of the melted wire ends to the other, completing the circuit again:
480 V
Load
blown
fuse
480 V
drop
When the fuse "blows," full
supply voltage will be dropped
across it and there will be no
current in the circuit.
480 V
Load
excessive
voltage
arc!
If the voltage across the blown
fuse is high enough, a spark may
jump the gap, allowing some
current in the circuit. THIS WOULD
NOT BE GOOD!!!
Consequently, fuses are rated in terms of their voltage capacity as well as the current level
at which they will blow.
Some large industrial fuses have replaceable wire elements, to reduce the expense. The
body of the fuse is an opaque, reusable cartridge, shielding the fuse wire from exposure and
shielding surrounding objects from the fuse wire.
There's more to the current rating of a fuse than a single number. If a current of 35 amps is
sent through a 30 amp fuse, it may blow suddenly or delay before blowing, depending on other
aspects of its design. Some fuses are intended to blow very fast, while others are designed for
more modest "opening" times, or even for a delayed action depending on the application. The
latter fuses are sometimes called slow-blow fuses due to their intentional time-delay charac-
teristics.
A classic example of a slow-blow fuse application is in electric motor protection, where in-
rush currents of up to ten times normal operating current are commonly experienced every
12.4.
FUSES
425
time the motor is started from a dead stop. If fast-blowing fuses were to be used in an applica-
tion like this, the motor could never get started because the normal inrush current levels would
blow the fuse(s) immediately! The design of a slow-blow fuse is such that the fuse element has
more mass (but no more ampacity) than an equivalent fast-blow fuse, meaning that it will heat
up slower (but to the same ultimate temperature) for any given amount of current.
On the other end of the fuse action spectrum, there are so-called semiconductor fuses de-
signed to open very quickly in the event of an overcurrent condition. Semiconductor devices
such as transistors tend to be especially intolerant of overcurrent conditions, and as such re-
quire fast-acting protection against overcurrents in high-power applications.
Fuses are always supposed to be placed on the "hot" side of the load in systems that are
grounded. The intent of this is for the load to be completely de-energized in all respects after
the fuse opens. To see the difference between fusing the "hot" side versus the "neutral" side of
a load, compare these two circuits:
load
blown fuse
"Hot"
"Neutral"
no voltage between either side
of load and ground
load
blown fuse
"Hot"
"Neutral"
voltage present between either side
of load and ground!
In either case, the fuse successfully interrupted current to the load, but the lower circuit
426
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
fails to interrupt potentially dangerous voltage from either side of the load to ground, where a
person might be standing. The first circuit design is much safer.
As it was said before, fuses are not the only type of overcurrent protection device in use.
Switch-like devices called circuit breakers are often (and more commonly) used to open circuits
with excessive current, their popularity due to the fact that they don't destroy themselves in
the process of breaking the circuit as fuses do. In any case, though, placement of the overcur-
rent protection device in a circuit will follow the same general guidelines listed above: namely,
to "fuse" the side of the power supply not connected to ground.
Although overcurrent protection placement in a circuit may determine the relative shock
hazard of that circuit under various conditions, it must be understood that such devices were
never intended to guard against electric shock. Neither fuses nor circuit breakers were de-
signed to open in the event of a person getting shocked; rather, they are intended to open
only under conditions of potential conductor overheating. Overcurrent devices primarily pro-
tect the conductors of a circuit from overtemperature damage (and the fire hazards associated
with overly hot conductors), and secondarily protect specific pieces of equipment such as loads
and generators (some fast-acting fuses are designed to protect electronic devices particularly
susceptible to current surges). Since the current levels necessary for electric shock or electro-
cution are much lower than the normal current levels of common power loads, a condition of
overcurrent is not indicative of shock occurring. There are other devices designed to detect
certain shock conditions (ground-fault detectors being the most popular), but these devices
strictly serve that one purpose and are uninvolved with protection of the conductors against
overheating.
• REVIEW:
• A fuse is a small, thin conductor designed to melt and separate into two pieces for the
purpose of breaking a circuit in the event of excessive current.
• A circuit breaker is a specially designed switch that automatically opens to interrupt
circuit current in the event of an overcurrent condition. They can be "tripped" (opened)
thermally, by magnetic fields, or by external devices called "protective relays," depending
on the design of breaker, its size, and the application.
• Fuses are primarily rated in terms of maximum current, but are also rated in terms of
how much voltage drop they will safely withstand after interrupting a circuit.
• Fuses can be designed to blow fast, slow, or anywhere in between for the same maximum
level of current.
• The best place to install a fuse in a grounded power system is on the ungrounded conduc-
tor path to the load. That way, when the fuse blows there will only be the grounded (safe)
conductor still connected to the load, making it safer for people to be around.
12.5
Specific resistance
Conductor ampacity rating is a crude assessment of resistance based on the potential for cur-
rent to create a fire hazard. However, we may come across situations where the voltage drop
12.5.
SPECIFIC RESISTANCE
427
created by wire resistance in a circuit poses concerns other than fire avoidance. For instance,
we may be designing a circuit where voltage across a component is critical, and must not fall
below a certain limit. If this is the case, the voltage drops resulting from wire resistance may
cause an engineering problem while being well within safe (fire) limits of ampacity:
wire resistance
wire resistance
Load
(requires at least 220 V)
2300 feet
230 V
25 A
25 A
If the load in the above circuit will not tolerate less than 220 volts, given a source voltage
of 230 volts, then we'd better be sure that the wiring doesn't drop more than 10 volts along the
way. Counting both the supply and return conductors of this circuit, this leaves a maximum
tolerable drop of 5 volts along the length of each wire. Using Ohm's Law (R=E/I), we can
determine the maximum allowable resistance for each piece of wire:
I
25 A
We know that the wire length is 2300 feet for each piece of wire, but how do we determine
the amount of resistance for a specific size and length of wire? To do that, we need another
formula:
A
This formula relates the resistance of a conductor with its specific resistance (the Greek
letter "rho" (ρ), which looks similar to a lower-case letter "p"), its length ("l"), and its cross-
sectional area ("A"). Notice that with the length variable on the top of the fraction, the re-
sistance value increases as the length increases (analogy: it is more difficult to force liquid
through a long pipe than a short one), and decreases as cross-sectional area increases (anal-
ogy: liquid flows easier through a fat pipe than through a skinny one). Specific resistance is a
constant for the type of conductor material being calculated.
The specific resistances of several conductive materials can be found in the following ta-
ble. We find copper near the bottom of the table, second only to silver in having low specific
resistance (good conductivity):
Specific resistance table: 12.4
428
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
Table 12.4: Specific resistance at 20o C:
Material
Element/Alloy
(ohm-cm·10−6)
Nichrome
Alloy
675
112.2
Nichrome V
Alloy
650
108.1
Manganin
Alloy
290
48.21
Constantan
Alloy
272.97
45.38
Alloy
100
16.62
Platinum
Element
63.16
10.5
Iron
Element
57.81
9.61
Nickel
Element
41.69
6.93
Zinc
Element
35.49
5.90
Molybdenum
Element
32.12
5.34
Tungsten
Element
31.76
5.28
Aluminum
Element
15.94
2.650
Gold
Element
13.32
2.214
Copper
Element
10.09
1.678
Silver
Element
9.546
1.587
* = Steel alloy at 99.5% iron, 0.5% carbon
Notice that the figures for specific resistance in the above table are given in the very strange
unit of "ohms-cmil/ft" (Ω-cmil/ft), This unit indicates what units we are expected to use in the
resistance formula (R=ρl/A). In this case, these figures for specific resistance are intended to
be used when length is measured in feet and cross-sectional area is measured in circular mils.
The metric unit for specific resistance is the ohm-meter (Ω-m), or ohm-centimeter (Ω-cm),
with 1.66243 x 10−9 Ω-meters per Ω-cmil/ft (1.66243 x 10−7 Ω-cm per Ω-cmil/ft). In the Ω-cm
column of the table, the figures are actually scaled as μΩ-cm due to their very small magni-
tudes. For example, iron is listed as 9.61 μΩ-cm, which could be represented as 9.61 x 10−6
When using the unit of Ω-meter for specific resistance in the R=ρl/A formula, the length
needs to be in meters and the area in square meters. When using the unit of Ω-centimeter
(Ω-cm) in the same formula, the length needs to be in centimeters and the area in square
centimeters.
All these units for specific resistance are valid for any material (Ω-cmil/ft, Ω-m, or Ω-cm).
One might prefer to use Ω-cmil/ft, however, when dealing with round wire where the cross-
sectional area is already known in circular mils. Conversely, when dealing with odd-shaped
busbar or custom busbar cut out of metal stock, where only the linear dimensions of length,
width, and height are known, the specific resistance units of Ω-meter or Ω-cm may be more
appropriate.
Going back to our example circuit, we were looking for wire that had 0.2 Ωor less of re-
sistance over a length of 2300 feet. Assuming that we're going to use copper wire (the most
common type of electrical wire manufactured), we can set up our formula as such:
12.5.
SPECIFIC RESISTANCE
429
A
. . . solving for unknown area . . .
A
R
2300 feet
0.2 Ω
A = 116,035 cmils
Algebraically solving for A, we get a value of 116,035 circular mils. Referencing our solid
wire size table, we find that "double-ought" (2/0) wire with 133,100 cmils is adequate, whereas
the next lower size, "single-ought" (1/0), at 105,500 cmils is too small. Bear in mind that our
circuit current is a modest 25 amps. According to our ampacity table for copper wire in free air,
14 gauge wire would have sufficed (as far as not starting a fire is concerned). However, from
the standpoint of voltage drop, 14 gauge wire would have been very unacceptable.
Just for fun, let's see what 14 gauge wire would have done to our power circuit's perfor-
mance. Looking at our wire size table, we find that 14 gauge wire has a cross-sectional area of
4,107 circular mils. If we're still using copper as a wire material (a good choice, unless we're
really rich and can afford 4600 feet of 14 gauge silver wire!), then our specific resistance will
still be 10.09 Ω-cmil/ft:
A
2300 feet
4107 cmil
Remember that this is 5.651 Ωper 2300 feet of 14-gauge copper wire, and that we have two
runs of 2300 feet in the entire circuit, so each wire piece in the circuit has 5.651 Ωof resistance:
wire resistance
wire resistance
Load
(requires at least 220 V)
2300 feet
230 V
5.651 Ω
5.651 Ω
Our total circuit wire resistance is 2 times 5.651, or 11.301 Ω. Unfortunately, this is far too
much resistance to allow 25 amps of current with a source voltage of 230 volts. Even if our
430
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
load resistance was 0 Ω, our wiring resistance of 11.301 Ωwould restrict the circuit current
to a mere 20.352 amps! As you can see, a "small" amount of wire resistance can make a big
difference in circuit performance, especially in power circuits where the currents are much
higher than typically encountered in electronic circuits.
Let's do an example resistance problem for a piece of custom-cut busbar. Suppose we have a
piece of solid aluminum bar, 4 centimeters wide by 3 centimeters tall by 125 centimeters long,
and we wish to figure the end-to-end resistance along the long dimension (125 cm). First, we
would need to determine the cross-sectional area of the bar:
Area = Width x Height
A = (4 cm)(3 cm)
A = 12 square cm
We also need to know the specific resistance of aluminum, in the unit proper for this ap-
plication (Ω-cm). From our table of specific resistances, we see that this is 2.65 x 10−6 Ω-cm.
Setting up our R=ρl/A formula, we have:
A
125 cm
12 cm2
R = 27.604 μΩ
As you can see, the sheer thickness of a busbar makes for very low resistances compared to
that of standard wire sizes, even when using a material with a greater specific resistance.
The procedure for determining busbar resistance is not fundamentally different than for
determining round wire resistance. We just need to make sure that cross-sectional area is
calculated properly and that all the units correspond to each other as they should.
• REVIEW:
• Conductor resistance increases with increased length and decreases with increased cross-
sectional area, all other factors being equal.
• Specific Resistance ("ρ") is a property of any conductive material, a figure used to deter-
mine the end-to-end resistance of a conductor given length and area in this formula: R =
• Specific resistance for materials are given in units of Ω-cmil/ft or Ω-meters (metric).
Conversion factor between these two units is 1.66243 x 10−9 Ω-meters per Ω-cmil/ft, or
1.66243 x 10−7 Ω-cm per Ω-cmil/ft.
• If wiring voltage drop in a circuit is critical, exact resistance calculations for the wires
must be made before wire size is chosen.
12.6.
TEMPERATURE COEFFICIENT OF RESISTANCE
431
12.6
Temperature coefficient of resistance
You might have noticed on the table for specific resistances that all figures were specified at a
temperature of 20o Celsius. If you suspected that this meant specific resistance of a material
may change with temperature, you were right!
Resistance values for conductors at any temperature other than the standard temperature
(usually specified at 20 Celsius) on the specific resistance table must be determined through
yet another formula:
Where,
R = Conductor resistance at temperature "T"
Rref = Conductor resistance at reference temperature
α = Temperature coefficient of resistance for the
conductor material.
T = Conductor temperature in degrees Celcius.
Tref = Reference temperature that α is specified at
for the conductor material.
Tref, usually 20o C, but sometimes 0o C.
The "alpha" (α) constant is known as the temperature coefficient of resistance, and sym-
bolizes the resistance change factor per degree of temperature change. Just as all materials
have a certain specific resistance (at 20o C), they also change resistance according to tempera-
ture by certain amounts. For pure metals, this coefficient is a positive number, meaning that
resistance increases with increasing temperature. For the elements carbon, silicon, and germa-
nium, this coefficient is a negative number, meaning that resistance decreases with increasing
temperature. For some metal alloys, the temperature coefficient of resistance is very close to
zero, meaning that the resistance hardly changes at all with variations in temperature (a good
property if you want to build a precision resistor out of metal wire!). The following table gives
the temperature coefficients of resistance for several common metals, both pure and alloy:
Temperature coefficient table: 12.5
Let's take a look at an example circuit to see how temperature can affect wire resistance,
and consequently circuit performance:
14 V
250 Ω
Rload
Rwire1 = 15 Ω
Temp = 20 C
432
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
Table 12.5: Temperature coefficient (α) per degree C:
Material
Element/Alloy
Temp. coefficient
Nickel
Element
0.005866
Iron
Element
0.005671
Molybdenum
Element
0.004579
Tungsten
Element
0.004403
Aluminum
Element
0.004308
Copper
Element
0.004041
Silver
Element
0.003819
Platinum
Element
0.003729
Gold
Element
0.003715
Zinc
Element
0.003847
Alloy
0.003
Nichrome
Alloy
0.00017
Nichrome V
Alloy
0.00013
Manganin
Alloy
0.000015
Constantan
Alloy
±0.000074
* = Steel alloy at 99.5% iron, 0.5% carbon
This circuit has a total wire resistance (wire 1 + wire 2) of 30 Ωat standard temperature.
Setting up a table of voltage, current, and resistance values we get:
E
I
R
Volts
Amps
Ohms
Total
Wire1
Wire2
Load
15
15
250
280
14
50 m
50 m
50 m
50 m
0.75
0.75
12.5
At 20o Celsius, we get 12.5 volts across the load and a total of 1.5 volts (0.75 + 0.75) dropped
across the wire resistance. If the temperature were to rise to 35o Celsius, we could easily
determine the change of resistance for each piece of wire. Assuming the use of copper wire (α
= 0.004041) we get:
R = 15.909 Ω
Recalculating our circuit values, we see what changes this increase in temperature will
bring:
12.6.
TEMPERATURE COEFFICIENT OF RESISTANCE
433
E
I
R
Volts
Amps
Ohms
Total
Wire1
Wire2
Load
250
14
15.909
15.909
281.82
49.677m
49.677m
49.677m
49.677m
0.79
0.79
12.42
As you can see, voltage across the load went down (from 12.5 volts to 12.42 volts) and
voltage drop across the wires went up (from 0.75 volts to 0.79 volts) as a result of the tem-
perature increasing. Though the changes may seem small, they can be significant for power
lines stretching miles between power plants and substations, substations and loads. In fact,
power utility companies often have to take line resistance changes resulting from seasonal
temperature variations into account when calculating allowable system loading.
• REVIEW:
• Most conductive materials change specific resistance with changes in temperature. This
is why figures of specific resistance are always specified at a standard temperature (usu-
ally 20o or 25o Celsius).
• The resistance-change factor per degree Celsius of temperature change is called the tem-
perature coefficient of resistance. This factor is represented by the Greek lower-case letter
"alpha" (α).
• A positive coefficient for a material means that its resistance increases with an increase in
temperature. Pure metals typically have positive temperature coefficients of resistance.
Coefficients approaching zero can be obtained by alloying certain metals.
• A negative coefficient for a material means that its resistance decreases with an increase
in temperature. Semiconductor materials (carbon, silicon, germanium) typically have
negative temperature coefficients of resistance.
• The formula used to determine the resistance of a conductor at some temperature other
than what is specified in a resistance table is as follows:
Where,
R = Conductor resistance at temperature "T"
Rref = Conductor resistance at reference temperature
α = Temperature coefficient of resistance for the
conductor material.
T = Conductor temperature in degrees Celcius.
Tref = Reference temperature that α is specified at
for the conductor material.
Tref, usually 20o C, but sometimes 0o C.
434
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
12.7
Superconductivity
Conductors lose all of their electrical resistance when cooled to super-low temperatures (near
absolute zero, about -273o Celsius). It must be understood that superconductivity is not merely
an extrapolation of most conductors' tendency to gradually lose resistance with decreasing
temperature; rather, it is a sudden, quantum leap in resistivity from finite to nothing.
A
superconducting material has absolutely zero electrical resistance, not just some small amount.
Superconductivity was first discovered by H. Kamerlingh Onnes at the University of Lei-
den, Netherlands in 1911. Just three years earlier, in 1908, Onnes had developed a method of
liquefying helium gas, which provided a medium with which to supercool experimental objects
to just a few degrees above absolute zero. Deciding to investigate changes in electrical resis-
tance of mercury when cooled to this low of a temperature, he discovered that its resistance
dropped to nothing just below the boiling point of helium.
There is some debate over exactly how and why superconducting materials superconduct.
One theory holds that electrons group together and travel in pairs (called Cooper pairs) within
a superconductor rather than travel independently, and that has something to do with their
frictionless flow. Interestingly enough, another phenomenon of super-cold temperatures, super-
fluidity, happens with certain liquids (especially liquid helium), resulting in frictionless flow of
molecules.
Superconductivity promises extraordinary capabilities for electric circuits. If conductor re-
sistance could be eliminated entirely, there would be no power losses or inefficiencies in electric
power systems due to stray resistances. Electric motors could be made almost perfectly (100%)
efficient. Components such as capacitors and inductors, whose ideal characteristics are nor-
mally spoiled by inherent wire resistances, could be made ideal in a practical sense. Already,
some practical superconducting conductors, motors, and capacitors have been developed, but
their use at this present time is limited due to the practical problems intrinsic to maintaining
super-cold temperatures.
The threshold temperature for a superconductor to switch from normal conduction to super-
conductivity is called the transition temperature. Transition temperatures for "classic" super-
conductors are in the cryogenic range (near absolute zero), but much progress has been made
in developing "high-temperature" superconductors which superconduct at warmer tempera-
tures. One type is a ceramic mixture of yttrium, barium, copper, and oxygen which transitions
at a relatively balmy -160o Celsius. Ideally, a superconductor should be able to operate within
the range of ambient temperatures, or at least within the range of inexpensive refrigeration
equipment.
The critical temperatures for a few common substances are shown here in this table. Tem-
peratures are given in kelvins, which has the same incremental span as degrees Celsius (an
increase or decrease of 1 kelvin is the same amount of temperature change as 1o Celsius), only
offset so that 0 K is absolute zero. This way, we don't have to deal with a lot of negative figures.
Critical temperature, superconductors 12.6
Superconducting materials also interact in interesting ways with magnetic fields. While in
the superconducting state, a superconducting material will tend to exclude all magnetic fields,
a phenomenon known as the Meissner effect. However, if the magnetic field strength intensifies
beyond a critical level, the superconducting material will be rendered non-superconductive. In
12.7.
SUPERCONDUCTIVITY
435
Table 12.6: Critical temperatures given in Kelvins
Material
Element or Alloy
Critical temperature(K)
Aluminum
Element
1.20
Cadmium
Element
0.56
Lead
Element
7.2
Mercury
Element
4.16
Niobium
Element
8.70
Thorium
Element
1.37
Tin
Element
3.72
Titanium
Element
0.39
Uranium
ELement
1.0
Zinc
Element
0.91
Niobium/Tin
Alloy
18.1
Cupric sulphide
Compound
1.6
other words, superconducting materials will lose their superconductivity (no matter how cold
you make them) if exposed to too strong of a magnetic field. In fact, the presence of any mag-
netic field tends to lower the critical temperature of any superconducting material: the more
magnetic field present, the colder you have to make the material before it will superconduct.
This is another practical limitation to superconductors in circuit design, since electric cur-
rent through any conductor produces a magnetic field. Even though a superconducting wire
would have zero resistance to oppose current, there will still be a limit of how much current
could practically go through that wire due to its critical magnetic field limit.
There are already a few industrial applications of superconductors, especially since the
recent (1987) advent of the yttrium-barium-copper-oxygen ceramic, which only requires liquid
nitrogen to cool, as opposed to liquid helium. It is even possible to order superconductivity
kits from educational suppliers which can be operated in high school labs (liquid nitrogen not
included). Typically, these kits exhibit superconductivity by the Meissner effect, suspending a
tiny magnet in mid-air over a superconducting disk cooled by a bath of liquid nitrogen.
The zero resistance offered by superconducting circuits leads to unique consequences. In a
superconducting short-circuit, it is possible to maintain large currents indefinitely with zero
applied voltage!
436
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
superconducting wire
electrons will flow unimpeded by
resistance, continuing to flow
forever!
Rings of superconducting material have been experimentally proven to sustain continuous
current for years with no applied voltage. So far as anyone knows, there is no theoretical
time limit to how long an unaided current could be sustained in a superconducting circuit.
If you're thinking this appears to be a form of perpetual motion, you're correct! Contrary to
popular belief, there is no law of physics prohibiting perpetual motion; rather, the prohibi-
tion stands against any machine or system generating more energy than it consumes (what
would be referred to as an over-unity device). At best, all a perpetual motion machine (like the
superconducting ring) would be good for is to store energy, not generate it freely!
Superconductors also offer some strange possibilities having nothing to do with Ohm's Law.
One such possibility is the construction of a device called a Josephson Junction, which acts as
a relay of sorts, controlling one current with another current (with no moving parts, of course).
The small size and fast switching time of Josephson Junctions may lead to new computer
circuit designs: an alternative to using semiconductor transistors.
• REVIEW:
• Superconductors are materials which have absolutely zero electrical resistance.
• All presently known superconductive materials need to be cooled far below ambient tem-
perature to superconduct. The maximum temperature at which they do so is called the
transition temperature.
12.8
Insulator breakdown voltage
The atoms in insulating materials have very tightly-bound electrons, resisting free electron
flow very well. However, insulators cannot resist indefinite amounts of voltage. With enough
voltage applied, any insulating material will eventually succumb to the electrical "pressure"
12.8.
INSULATOR BREAKDOWN VOLTAGE
437
and electron flow will occur. However, unlike the situation with conductors where current is in
a linear proportion to applied voltage (given a fixed resistance), current through an insulator
is quite nonlinear: for voltages below a certain threshold level, virtually no electrons will flow,
but if the voltage exceeds that threshold, there will be a rush of current.
Once current is forced through an insulating material, breakdown of that material's molec-
ular structure has occurred. After breakdown, the material may or may not behave as an in-
sulator any more, the molecular structure having been altered by the breach. There is usually
a localized "puncture" of the insulating medium where the electrons flowed during breakdown.
Thickness of an insulating material plays a role in determining its breakdown voltage,
otherwise known as dielectric strength. Specific dielectric strength is sometimes listed in terms
of volts per mil (1/1000 of an inch), or kilovolts per inch (the two units are equivalent), but in
practice it has been found that the relationship between breakdown voltage and thickness is
not exactly linear. An insulator three times as thick has a dielectric strength slightly less than
3 times as much. However, for rough estimation use, volt-per-thickness ratings are fine.
Dielectric strength: 12.7
Table 12.7: Dielectric strength in kilovolts per inch (kV/in):
Material*
Dielectric strength
Vacuum
20
Air
20 to 75
Porcelain
40 to 200
Paraffin Wax
200 to 300
Transformer Oil
400
Bakelite
300 to 550
Rubber
450 to 700
Shellac
900
Paper
1250
Teflon
1500
Glass
2000 to 3000
Mica
5000
* = Materials listed are specially prepared for electrical use, 12.7.
• REVIEW:
• With a high enough applied voltage, electrons can be freed from the atoms of insulating
materials, resulting in current through that material.
• The minimum voltage required to "violate" an insulator by forcing current through it is
called the breakdown voltage, or dielectric strength.
• The thicker a piece of insulating material, the higher the breakdown voltage, all other
factors being equal.
• Specific dielectric strength is typically rated in one of two equivalent units: volts per mil,
or kilovolts per inch.
438
CHAPTER 12. PHYSICS OF CONDUCTORS AND INSULATORS
12.9
Data
Tables of specific resistance and temperature coefficient of resistance for elemental materi-
als (not alloys) were derived from figures found in the 78th edition of the CRC Handbook of
Chemistry and Physics.
Table of superconductor critical temperatures derived from figures found in the 21st volume
of Collier's Encyclopedia, 1968.
12.10
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Aaron Forster (February 18, 2003): Typographical error correction.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 13
CAPACITORS
Contents
13.1 Electric fields and capacitance
. . . . . . . . . . . . . . . . . . . . . . . . . . 439
13.2 Capacitors and calculus . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 444
13.3 Factors affecting capacitance . . . . . . . . . . . . . . . . . . . . . . . . . . . 449
13.4 Series and parallel capacitors . . . . . . . . . . . . . . . . . . . . . . . . . . . 453
13.5 Practical considerations
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 454
13.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 459
13.1
Electric fields and capacitance
Whenever an electric voltage exists between two separated conductors, an electric field is
present within the space between those conductors. In basic electronics, we study the interac-
tions of voltage, current, and resistance as they pertain to circuits, which are conductive paths
through which electrons may travel. When we talk about fields, however, we're dealing with
interactions that can be spread across empty space.
Admittedly, the concept of a "field" is somewhat abstract. At least with electric current it
isn't too difficult to envision tiny particles called electrons moving their way between the nuclei
of atoms within a conductor, but a "field" doesn't even have mass, and need not exist within
matter at all.
Despite its abstract nature, almost every one of us has direct experience with fields, at
least in the form of magnets. Have you ever played with a pair of magnets, noticing how they
attract or repel each other depending on their relative orientation? There is an undeniable
force between a pair of magnets, and this force is without "substance." It has no mass, no
color, no odor, and if not for the physical force exerted on the magnets themselves, it would
be utterly insensible to our bodies. Physicists describe the interaction of magnets in terms of
magnetic fields in the space between them. If iron filings are placed near a magnet, they orient
themselves along the lines of the field, visually indicating its presence.
439
440
CHAPTER 13. CAPACITORS
The subject of this chapter is electric fields (and devices called capacitors that exploit them),
not magnetic fields, but there are many similarities. Most likely you have experienced electric
fields as well. Chapter 1 of this book began with an explanation of static electricity, and how
materials such as wax and wool - when rubbed against each other - produced a physical attrac-
tion. Again, physicists would describe this interaction in terms of electric fields generated by
the two objects as a result of their electron imbalances. Suffice it to say that whenever a volt-
age exists between two points, there will be an electric field manifested in the space between
those points.
Fields have two measures: a field force and a field flux. The field force is the amount of
"push" that a field exerts over a certain distance. The field flux is the total quantity, or effect,
of the field through space. Field force and flux are roughly analogous to voltage ("push") and
current (flow) through a conductor, respectively, although field flux can exist in totally empty
space (without the motion of particles such as electrons) whereas current can only take place
where there are free electrons to move. Field flux can be opposed in space, just as the flow
of electrons can be opposed by resistance. The amount of field flux that will develop in space
is proportional to the amount of field force applied, divided by the amount of opposition to
flux. Just as the type of conducting material dictates that conductor's specific resistance to
electric current, the type of insulating material separating two conductors dictates the specific
opposition to field flux.
Normally, electrons cannot enter a conductor unless there is a path for an equal amount
of electrons to exit (remember the marble-in-tube analogy?). This is why conductors must be
connected together in a circular path (a circuit) for continuous current to occur. Oddly enough,
however, extra electrons can be "squeezed" into a conductor without a path to exit if an electric
field is allowed to develop in space relative to another conductor. The number of extra free
electrons added to the conductor (or free electrons taken away) is directly proportional to the
amount of field flux between the two conductors.
Capacitors are components designed to take advantage of this phenomenon by placing two
conductive plates (usually metal) in close proximity with each other. There are many different
styles of capacitor construction, each one suited for particular ratings and purposes. For very
small capacitors, two circular plates sandwiching an insulating material will suffice. For larger
capacitor values, the "plates" may be strips of metal foil, sandwiched around a flexible insu-
lating medium and rolled up for compactness. The highest capacitance values are obtained by
using a microscopic-thickness layer of insulating oxide separating two conductive surfaces. In
any case, though, the general idea is the same: two conductors, separated by an insulator.
The schematic symbol for a capacitor is quite simple, being little more than two short,
parallel lines (representing the plates) separated by a gap. Wires attach to the respective
plates for connection to other components. An older, obsolete schematic symbol for capacitors
showed interleaved plates, which is actually a more accurate way of representing the real
construction of most capacitors:
13.1.
ELECTRIC FIELDS AND CAPACITANCE
441
modern
obsolete
Capacitor symbols
When a voltage is applied across the two plates of a capacitor, a concentrated field flux is
created between them, allowing a significant difference of free electrons (a charge) to develop
between the two plates:
excess free electrons
deficiency of electrons
metal plate
metal plate
electric field
As the electric field is established by the applied voltage, extra free electrons are forced to
collect on the negative conductor, while free electrons are "robbed" from the positive conductor.
This differential charge equates to a storage of energy in the capacitor, representing the poten-
tial charge of the electrons between the two plates. The greater the difference of electrons on
opposing plates of a capacitor, the greater the field flux, and the greater "charge" of energy the
capacitor will store.
Because capacitors store the potential energy of accumulated electrons in the form of an
electric field, they behave quite differently than resistors (which simply dissipate energy in the
form of heat) in a circuit. Energy storage in a capacitor is a function of the voltage between the
plates, as well as other factors which we will discuss later in this chapter. A capacitor's ability
to store energy as a function of voltage (potential difference between the two leads) results in
a tendency to try to maintain voltage at a constant level. In other words, capacitors tend to
resist changes in voltage drop. When voltage across a capacitor is increased or decreased, the
capacitor "resists" the change by drawing current from or supplying current to the source of
the voltage change, in opposition to the change.
To store more energy in a capacitor, the voltage across it must be increased. This means
that more electrons must be added to the (-) plate and more taken away from the (+) plate,
necessitating a current in that direction. Conversely, to release energy from a capacitor, the
voltage across it must be decreased. This means some of the excess electrons on the (-) plate
must be returned to the (+) plate, necessitating a current in the other direction.
Just as Isaac Newton's first Law of Motion ("an object in motion tends to stay in motion;
an object at rest tends to stay at rest") describes the tendency of a mass to oppose changes in
velocity, we can state a capacitor's tendency to oppose changes in voltage as such: "A charged
442
CHAPTER 13. CAPACITORS
capacitor tends to stay charged; a discharged capacitor tends to stay discharged." Hypothet-
ically, a capacitor left untouched will indefinitely maintain whatever state of voltage charge
that its been left with. Only an outside source (or drain) of current can alter the voltage charge
stored by a perfect capacitor:
voltage (charge) sustained with
the capacitor open-circuited
C
Practically speaking, however, capacitors will eventually lose their stored voltage charges
due to internal leakage paths for electrons to flow from one plate to the other. Depending on
the specific type of capacitor, the time it takes for a stored voltage charge to self-dissipate can
be a long time (several years with the capacitor sitting on a shelf!).
When the voltage across a capacitor is increased, it draws current from the rest of the
circuit, acting as a power load. In this condition the capacitor is said to be charging, because
there is an increasing amount of energy being stored in its electric field. Note the direction of
electron current with regard to the voltage polarity:
C
. . . to the rest of
the circuit
I
I
increasing
voltage
Energy being absorbed by
the capacitor from the rest
of the circuit.
The capacitor acts as a LOAD
Conversely, when the voltage across a capacitor is decreased, the capacitor supplies current
to the rest of the circuit, acting as a power source. In this condition the capacitor is said to
be discharging. Its store of energy - held in the electric field - is decreasing now as energy
is released to the rest of the circuit. Note the direction of electron current with regard to the
voltage polarity:
13.1.
ELECTRIC FIELDS AND CAPACITANCE
443
C
. . . to the rest of
the circuit
I
I
voltage
The capacitor acts as a SOURCE
Energy being released by the
capacitor to the rest of the circuit
decreasing
If a source of voltage is suddenly applied to an uncharged capacitor (a sudden increase
of voltage), the capacitor will draw current from that source, absorbing energy from it, until
the capacitor's voltage equals that of the source. Once the capacitor voltage reaches this final
(charged) state, its current decays to zero. Conversely, if a load resistance is connected to a
charged capacitor, the capacitor will supply current to the load, until it has released all its
stored energy and its voltage decays to zero. Once the capacitor voltage reaches this final
(discharged) state, its current decays to zero. In their ability to be charged and discharged,
capacitors can be thought of as acting somewhat like secondary-cell batteries.
The choice of insulating material between the plates, as was mentioned before, has a great
impact upon how much field flux (and therefore how much charge) will develop with any given
amount of voltage applied across the plates. Because of the role of this insulating material in
affecting field flux, it has a special name: dielectric. Not all dielectric materials are equal: the
extent to which materials inhibit or encourage the formation of electric field flux is called the
permittivity of the dielectric.
The measure of a capacitor's ability to store energy for a given amount of voltage drop is
called capacitance. Not surprisingly, capacitance is also a measure of the intensity of opposition
to changes in voltage (exactly how much current it will produce for a given rate of change in
voltage). Capacitance is symbolically denoted with a capital "C," and is measured in the unit
of the Farad, abbreviated as "F."
Convention, for some odd reason, has favored the metric prefix "micro" in the measurement
of large capacitances, and so many capacitors are rated in terms of confusingly large micro-
Farad values: for example, one large capacitor I have seen was rated 330,000 microFarads!!
Why not state it as 330 milliFarads? I don't know.
An obsolete name for a capacitor is condenser or condensor. These terms are not used in
any new books or schematic diagrams (to my knowledge), but they might be encountered in
older electronics literature. Perhaps the most well-known usage for the term "condenser" is
in automotive engineering, where a small capacitor called by that name was used to mitigate
excessive sparking across the switch contacts (called "points") in electromechanical ignition
systems.
• REVIEW:
• Capacitors react against changes in voltage by supplying or drawing current in the direc-
tion necessary to oppose the change.
444
CHAPTER 13. CAPACITORS
• When a capacitor is faced with an increasing voltage, it acts as a load: drawing current
as it absorbs energy (current going in the negative side and out the positive side, like a
resistor).
• When a capacitor is faced with a decreasing voltage, it acts as a source: supplying current
as it releases stored energy (current going out the negative side and in the positive side,
like a battery).
• The ability of a capacitor to store energy in the form of an electric field (and consequently
to oppose changes in voltage) is called capacitance. It is measured in the unit of the Farad
(F).
• Capacitors used to be commonly known by another term: condenser (alternatively spelled
"condensor").
13.2
Capacitors and calculus
Capacitors do not have a stable "resistance" as conductors do. However, there is a definite
mathematical relationship between voltage and current for a capacitor, as follows:
dv
dt
Where,
C
C = Capacitance in Farads
dv
dt
= Instantaneous rate of voltage change
(volts per second)
"Ohm's Law" for a capacitor
i = Instantaneous current through the capacitor
The lower-case letter "i" symbolizes instantaneous current, which means the amount of
current at a specific point in time. This stands in contrast to constant current or average
current (capital letter "I") over an unspecified period of time. The expression "dv/dt" is one
borrowed from calculus, meaning the instantaneous rate of voltage change over time, or the
rate of change of voltage (volts per second increase or decrease) at a specific point in time,
the same specific point in time that the instantaneous current is referenced at. For whatever
reason, the letter v is usually used to represent instantaneous voltage rather than the letter
e. However, it would not be incorrect to express the instantaneous voltage rate-of-change as
"de/dt" instead.
In this equation we see something novel to our experience thus far with electric circuits: the
variable of time. When relating the quantities of voltage, current, and resistance to a resistor,
it doesn't matter if we're dealing with measurements taken over an unspecified period of time
(E=IR; V=IR), or at a specific moment in time (e=ir; v=ir). The same basic formula holds true,
because time is irrelevant to voltage, current, and resistance in a component like a resistor.
13.2.
CAPACITORS AND CALCULUS
445
In a capacitor, however, time is an essential variable, because current is related to how
rapidly voltage changes over time. To fully understand this, a few illustrations may be neces-
sary. Suppose we were to connect a capacitor to a variable-voltage source, constructed with a
potentiometer and a battery:
V
Ammeter
(zero-center)
If the potentiometer mechanism remains in a single position (wiper is stationary), the volt-
meter connected across the capacitor will register a constant (unchanging) voltage, and the
ammeter will register 0 amps. In this scenario, the instantaneous rate of voltage change (dv/dt)
is equal to zero, because the voltage is unchanging. The equation tells us that with 0 volts per
second change for a dv/dt, there must be zero instantaneous current (i). From a physical per-
spective, with no change in voltage, there is no need for any electron motion to add or subtract
charge from the capacitor's plates, and thus there will be no current.
Time
Time
Capacitor
voltage
Capacitor
current
EC
IC
Potentiometer wiper not moving
Now, if the potentiometer wiper is moved slowly and steadily in the "up" direction, a greater
voltage will gradually be imposed across the capacitor. Thus, the voltmeter indication will be
increasing at a slow rate:
446
CHAPTER 13. CAPACITORS
V
Potentiometer wiper moving
slowly in the "up" direction
Increasing
Steady current
voltage
If we assume that the potentiometer wiper is being moved such that the rate of voltage
increase across the capacitor is steady (for example, voltage increasing at a constant rate of 2
volts per second), the dv/dt term of the formula will be a fixed value. According to the equa-
tion, this fixed value of dv/dt, multiplied by the capacitor's capacitance in Farads (also fixed),
results in a fixed current of some magnitude. From a physical perspective, an increasing volt-
age across the capacitor demands that there be an increasing charge differential between the
plates. Thus, for a slow, steady voltage increase rate, there must be a slow, steady rate of
charge building in the capacitor, which equates to a slow, steady flow rate of electrons, or cur-
rent. In this scenario, the capacitor is acting as a load, with electrons entering the negative
plate and exiting the positive, accumulating energy in the electric field.
Time
Time
Capacitor
voltage
Capacitor
current
EC
IC
Voltage
change
Time
Potentiometer wiper moving slowly "up"
If the potentiometer is moved in the same direction, but at a faster rate, the rate of voltage
change (dv/dt) will be greater and so will be the capacitor's current:
13.2.
CAPACITORS AND CALCULUS
447
V
Potentiometer wiper moving
Increasing
Steady current
voltage
quickly in the "up" direction
(greater)
(faster)
Time
Time
Capacitor
voltage
Capacitor
current
EC
IC
Voltage
change
Time
Potentiometer wiper moving quickly "up"
When mathematics students first study calculus, they begin by exploring the concept of
rates of change for various mathematical functions. The derivative, which is the first and most
elementary calculus principle, is an expression of one variable's rate of change in terms of
another. Calculus students have to learn this principle while studying abstract equations. You
get to learn this principle while studying something you can relate to: electric circuits!
To put this relationship between voltage and current in a capacitor in calculus terms, the
current through a capacitor is the derivative of the voltage across the capacitor with respect to
time. Or, stated in simpler terms, a capacitor's current is directly proportional to how quickly
the voltage across it is changing. In this circuit where capacitor voltage is set by the posi-
tion of a rotary knob on a potentiometer, we can say that the capacitor's current is directly
proportional to how quickly we turn the knob.
448
CHAPTER 13. CAPACITORS
If we were to move the potentiometer's wiper in the same direction as before ("up"), but at
varying rates, we would obtain graphs that looked like this:
Time
Time
Capacitor
voltage
Capacitor
current
EC
IC
Potentiometer wiper moving "up" at
different rates
Note how that at any given point in time, the capacitor's current is proportional to the rate-
of-change, or slope of the capacitor's voltage plot. When the voltage plot line is rising quickly
(steep slope), the current will likewise be great. Where the voltage plot has a mild slope, the
current is small. At one place in the voltage plot where it levels off (zero slope, representing a
period of time when the potentiometer wasn't moving), the current falls to zero.
If we were to move the potentiometer wiper in the "down" direction, the capacitor voltage
would decrease rather than increase. Again, the capacitor will react to this change of voltage
by producing a current, but this time the current will be in the opposite direction. A decreas-
ing capacitor voltage requires that the charge differential between the capacitor's plates be
reduced, and the only way that can happen is if the electrons reverse their direction of flow, the
capacitor discharging rather than charging. In this condition, with electrons exiting the nega-
tive plate and entering the positive, the capacitor will act as a source, like a battery, releasing
its stored energy to the rest of the circuit.
13.3.
FACTORS AFFECTING CAPACITANCE
449
V
Potentiometer wiper moving
voltage
in the "down" direction
Decreasing
Again, the amount of current through the capacitor is directly proportional to the rate of
voltage change across it. The only difference between the effects of a decreasing voltage and
an increasing voltage is the direction of electron flow. For the same rate of voltage change
over time, either increasing or decreasing, the current magnitude (amps) will be the same.
Mathematically, a decreasing voltage rate-of-change is expressed as a negative dv/dt quantity.
Following the formula i = C(dv/dt), this will result in a current figure (i) that is likewise nega-
tive in sign, indicating a direction of flow corresponding to discharge of the capacitor.
13.3
Factors affecting capacitance
There are three basic factors of capacitor construction determining the amount of capacitance
created. These factors all dictate capacitance by affecting how much electric field flux (relative
difference of electrons between plates) will develop for a given amount of electric field force
(voltage between the two plates):
PLATE AREA: All other factors being equal, greater plate area gives greater capacitance;
less plate area gives less capacitance.
Explanation: Larger plate area results in more field flux (charge collected on the plates) for
a given field force (voltage across the plates).
less capacitance
more capacitance
PLATE SPACING: All other factors being equal, further plate spacing gives less capaci-
tance; closer plate spacing gives greater capacitance.
Explanation: Closer spacing results in a greater field force (voltage across the capacitor di-
vided by the distance between the plates), which results in a greater field flux (charge collected
on the plates) for any given voltage applied across the plates.
450
CHAPTER 13. CAPACITORS
less capacitance
more capacitance
DIELECTRIC MATERIAL: All other factors being equal, greater permittivity of the di-
electric gives greater capacitance; less permittivity of the dielectric gives less capacitance.
Explanation: Although its complicated to explain, some materials offer less opposition to
field flux for a given amount of field force. Materials with a greater permittivity allow for more
field flux (offer less opposition), and thus a greater collected charge, for any given amount of
field force (applied voltage).
less capacitance
more capacitance
glass
air
(relative permittivity
(relative permittivity
"Relative" permittivity means the permittivity of a material, relative to that of a pure vac-
uum. The greater the number, the greater the permittivity of the material. Glass, for instance,
with a relative permittivity of 7, has seven times the permittivity of a pure vacuum, and conse-
quently will allow for the establishment of an electric field flux seven times stronger than that
of a vacuum, all other factors being equal.
The following is a table listing the relative permittivities (also known as the "dielectric
constant") of various common substances:
Relative permittivity table: 13.1
An approximation of capacitance for any pair of separated conductors can be found with
this formula:
Where,
d
ε A
C = Capacitance in Farads
ε = Permittivity of dielectric (absolute, not
relative)
A = Area of plate overlap in square meters
Distance between plates in meters
A formula for capacitance in picofarads using practical dimensions:
13.3.
FACTORS AFFECTING CAPACITANCE
451
Table 13.1: Relative permittivity
Material
Relative permittivity (dielectric constant)
Vacuum
1.0000
Air
1.0006
PTFE, FEP ("Teflon")
2.0
Polypropylene
2.20 to 2.28
ABS resin
2.4 to 3.2
Polystyrene
2.45 to 4.0
Waxed paper
2.5
Transformer oil
2.5 to 4
Hard Rubber
2.5 to 4.80
Wood (Oak)
3.3
Silicones
3.4 to 4.3
Bakelite
Quartz, fused
3.8
Wood (Maple)
4.4
Glass
4.9 to 7.5
Castor oil
5.0
Wood (Birch)
5.2
Mica, muscovite
5.0 to 8.7
Glass-bonded mica
6.3 to 9.3
Porcelain, Steatite
6.5
Alumina
8.0 to 10.0
Distilled water
80.0
Barium-strontium-titanite
7500
452
CHAPTER 13. CAPACITORS
Where,
d
0.0885K(n-1) A
Capacitance in picofarads
Dielectric constant
d'
Area of one plate in square centimeters
Area of one plate in square inches
Thickness in centimeters
Thickness in inches
Number of plates
0.225K(n-1)A'
d
A
A capacitor can be made variable rather than fixed in value by varying any of the physical
factors determining capacitance. One relatively easy factor to vary in capacitor construction is
that of plate area, or more properly, the amount of plate overlap.
The following photograph shows an example of a variable capacitor using a set of inter-
leaved metal plates and an air gap as the dielectric material:
As the shaft is rotated, the degree to which the sets of plates overlap each other will vary,
changing the effective area of the plates between which a concentrated electric field can be
established. This particular capacitor has a capacitance in the picofarad range, and finds use
in radio circuitry.
13.4.
SERIES AND PARALLEL CAPACITORS
453
13.4
Series and parallel capacitors
When capacitors are connected in series, the total capacitance is less than any one of the
series capacitors' individual capacitances. If two or more capacitors are connected in series,
the overall effect is that of a single (equivalent) capacitor having the sum total of the plate
spacings of the individual capacitors. As we've just seen, an increase in plate spacing, with all
other factors unchanged, results in decreased capacitance.
C1
C2
equivalent to
Ctotal
Thus, the total capacitance is less than any one of the individual capacitors' capacitances.
The formula for calculating the series total capacitance is the same form as for calculating
parallel resistances:
Series Capacitances
C1
C2
Cn
1
1
When capacitors are connected in parallel, the total capacitance is the sum of the individual
capacitors' capacitances. If two or more capacitors are connected in parallel, the overall effect
is that of a single equivalent capacitor having the sum total of the plate areas of the individual
capacitors. As we've just seen, an increase in plate area, with all other factors unchanged,
results in increased capacitance.
C1
C2
equivalent to
Ctotal
Thus, the total capacitance is more than any one of the individual capacitors' capacitances.
The formula for calculating the parallel total capacitance is the same form as for calculating
series resistances:
Parallel Capacitances
Ctotal = C1
C2
Cn
454
CHAPTER 13. CAPACITORS
As you will no doubt notice, this is exactly opposite of the phenomenon exhibited by re-
sistors. With resistors, series connections result in additive values while parallel connections
result in diminished values. With capacitors, its the reverse: parallel connections result in
additive values while series connections result in diminished values.
• REVIEW:
• Capacitances diminish in series.
• Capacitances add in parallel.
13.5
Practical considerations
Capacitors, like all electrical components, have limitations which must be respected for the
sake of reliability and proper circuit operation.
Working voltage: Since capacitors are nothing more than two conductors separated by an
insulator (the dielectric), you must pay attention to the maximum voltage allowed across it. If
too much voltage is applied, the "breakdown" rating of the dielectric material may be exceeded,
resulting in the capacitor internally short-circuiting.
Polarity: Some capacitors are manufactured so they can only tolerate applied voltage in one
polarity but not the other. This is due to their construction: the dielectric is a microscopically
thin layer of insulation deposited on one of the plates by a DC voltage during manufacture.
These are called electrolytic capacitors, and their polarity is clearly marked.
curved side of symbol is
always negative!
Electrolytic ("polarized")
capacitor
Reversing voltage polarity to an electrolytic capacitor may result in the destruction of that
super-thin dielectric layer, thus ruining the device. However, the thinness of that dielectric
permits extremely high values of capacitance in a relatively small package size. For the same
reason, electrolytic capacitors tend to be low in voltage rating as compared with other types of
capacitor construction.
Equivalent circuit: Since the plates in a capacitor have some resistance, and since no di-
electric is a perfect insulator, there is no such thing as a "perfect" capacitor. In real life, a
capacitor has both a series resistance and a parallel (leakage) resistance interacting with its
purely capacitive characteristics:
13.5.
PRACTICAL CONSIDERATIONS
455
Rseries
Rleakage
Capacitor equivalent circuit
Cideal
Fortunately, it is relatively easy to manufacture capacitors with very small series resis-
tances and very high leakage resistances!
Physical Size: For most applications in electronics, minimum size is the goal for component
engineering.
The smaller components can be made, the more circuitry can be built into a
smaller package, and usually weight is saved as well. With capacitors, there are two major
limiting factors to the minimum size of a unit: working voltage and capacitance. And these
two factors tend to be in opposition to each other. For any given choice in dielectric materials,
the only way to increase the voltage rating of a capacitor is to increase the thickness of the
dielectric. However, as we have seen, this has the effect of decreasing capacitance. Capacitance
can be brought back up by increasing plate area. but this makes for a larger unit. This is why
you cannot judge a capacitor's rating in Farads simply by size. A capacitor of any given size may
be relatively high in capacitance and low in working voltage, vice versa, or some compromise
between the two extremes. Take the following two photographs for example:
This is a fairly large capacitor in physical size, but it has quite a low capacitance value:
only 2 μF. However, its working voltage is quite high: 2000 volts! If this capacitor were re-
engineered to have a thinner layer of dielectric between its plates, at least a hundredfold in-
crease in capacitance might be achievable, but at a cost of significantly lowering its working
456
CHAPTER 13. CAPACITORS
voltage. Compare the above photograph with the one below. The capacitor shown in the lower
picture is an electrolytic unit, similar in size to the one above, but with very different values of
capacitance and working voltage:
The thinner dielectric layer gives it a much greater capacitance (20,000 μF) and a drasti-
cally reduced working voltage (35 volts continuous, 45 volts intermittent).
Here are some samples of different capacitor types, all smaller than the units shown previ-
ously:
13.5.
PRACTICAL CONSIDERATIONS
457
The electrolytic and tantalum capacitors are polarized (polarity sensitive), and are always
labeled as such. The electrolytic units have their negative (-) leads distinguished by arrow
symbols on their cases. Some polarized capacitors have their polarity designated by marking
the positive terminal. The large, 20,000 μF electrolytic unit shown in the upright position
has its positive (+) terminal labeled with a "plus" mark. Ceramic, mylar, plastic film, and air
capacitors do not have polarity markings, because those types are nonpolarized (they are not
polarity sensitive).
Capacitors are very common components in electronic circuits. Take a close look at the
following photograph - every component marked with a "C" designation on the printed circuit
board is a capacitor:
458
CHAPTER 13. CAPACITORS
Some of the capacitors shown on this circuit board are standard electrolytic: C30 (top of
board, center) and C36 (left side, 1/3 from the top). Some others are a special kind of electrolytic
capacitor called tantalum, because this is the type of metal used to make the plates. Tantalum
capacitors have relatively high capacitance for their physical size. The following capacitors on
the circuit board shown above are tantalum: C14 (just to the lower-left of C30), C19 (directly
below R10, which is below C30), C24 (lower-left corner of board), and C22 (lower-right).
Examples of even smaller capacitors can be seen in this photograph:
13.6.
CONTRIBUTORS
459
The capacitors on this circuit board are "surface mount devices" as are all the resistors,
for reasons of saving space. Following component labeling convention, the capacitors can be
identified by labels beginning with the letter "C".
13.6
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Warren Young (August 2002): Photographs of different capacitor types.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
460
CHAPTER 13. CAPACITORS
Chapter 14
MAGNETISM AND
ELECTROMAGNETISM
Contents
14.1 Permanent magnets
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 461
14.2 Electromagnetism
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 465
14.3 Magnetic units of measurement . . . . . . . . . . . . . . . . . . . . . . . . . . 467
14.4 Permeability and saturation . . . . . . . . . . . . . . . . . . . . . . . . . . . . 470
14.5 Electromagnetic induction . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 475
14.6 Mutual inductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 477
14.7 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 479
14.1
Permanent magnets
Centuries ago, it was discovered that certain types of mineral rock possessed unusual prop-
erties of attraction to the metal iron. One particular mineral, called lodestone, or magnetite,
is found mentioned in very old historical records (about 2500 years ago in Europe, and much
earlier in the Far East) as a subject of curiosity. Later, it was employed in the aid of navigation,
as it was found that a piece of this unusual rock would tend to orient itself in a north-south
direction if left free to rotate (suspended on a string or on a float in water). A scientific study
undertaken in 1269 by Peter Peregrinus revealed that steel could be similarly "charged" with
this unusual property after being rubbed against one of the "poles" of a piece of lodestone.
Unlike electric charges (such as those observed when amber is rubbed against cloth), mag-
netic objects possessed two poles of opposite effect, denoted "north" and "south" after their
self-orientation to the earth. As Peregrinus found, it was impossible to isolate one of these
poles by itself by cutting a piece of lodestone in half: each resulting piece possessed its own
pair of poles:
461
462
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
N
S
magnet
N
N
S
S
magnet
magnet
. . . after breaking in half . . .
Like electric charges, there were only two types of poles to be found: north and south (by
analogy, positive and negative). Just as with electric charges, same poles repel one another,
while opposite poles attract. This force, like that caused by static electricity, extended itself
invisibly over space, and could even pass through objects such as paper and wood with little
effect upon strength.
The philosopher-scientist Rene Descartes noted that this invisible "field" could be mapped
by placing a magnet underneath a flat piece of cloth or wood and sprinkling iron filings on
top. The filings will align themselves with the magnetic field, "mapping" its shape. The result
shows how the field continues unbroken from one pole of a magnet to the other:
N
S
magnet
magnetic field
As with any kind of field (electric, magnetic, gravitational), the total quantity, or effect, of
the field is referred to as a flux, while the "push" causing the flux to form in space is called a
force. Michael Faraday coined the term "tube" to refer to a string of magnetic flux in space (the
14.1.
PERMANENT MAGNETS
463
term "line" is more commonly used now). Indeed, the measurement of magnetic field flux is
often defined in terms of the number of flux lines, although it is doubtful that such fields exist
in individual, discrete lines of constant value.
Modern theories of magnetism maintain that a magnetic field is produced by an electric
charge in motion, and thus it is theorized that the magnetic field of a so-called "permanent"
magnets such as lodestone is the result of electrons within the atoms of iron spinning uni-
formly in the same direction. Whether or not the electrons in a material's atoms are subject
to this kind of uniform spinning is dictated by the atomic structure of the material (not unlike
how electrical conductivity is dictated by the electron binding in a material's atoms). Thus,
only certain types of substances react with magnetic fields, and even fewer have the ability to
permanently sustain a magnetic field.
Iron is one of those types of substances that readily magnetizes. If a piece of iron is brought
near a permanent magnet, the electrons within the atoms in the iron orient their spins to
match the magnetic field force produced by the permanent magnet, and the iron becomes "mag-
netized." The iron will magnetize in such a way as to incorporate the magnetic flux lines into its
shape, which attracts it toward the permanent magnet, no matter which pole of the permanent
magnet is offered to the iron:
N
S
magnet
magnetic field
iron
(unmagnetized)
The previously unmagnetized iron becomes magnetized as it is brought closer to the per-
manent magnet. No matter what pole of the permanent magnet is extended toward the iron,
the iron will magnetize in such a way as to be attracted toward the magnet:
464
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
N
S
magnet
iron
N
S
attraction
Referencing the natural magnetic properties of iron (Latin = "ferrum"), a ferromagnetic
material is one that readily magnetizes (its constituent atoms easily orient their electron spins
to conform to an external magnetic field force). All materials are magnetic to some degree,
and those that are not considered ferromagnetic (easily magnetized) are classified as either
paramagnetic (slightly magnetic) or diamagnetic (tend to exclude magnetic fields). Of the two,
diamagnetic materials are the strangest. In the presence of an external magnetic field, they
actually become slightly magnetized in the opposite direction, so as to repel the external field!
N
S
magnet
diamagnetic
material
N
S
repulsion
If a ferromagnetic material tends to retain its magnetization after an external field is re-
moved, it is said to have good retentivity. This, of course, is a necessary quality for a permanent
magnet.
• REVIEW:
14.2.
ELECTROMAGNETISM
465
• Lodestone (also called Magnetite) is a naturally-occurring "permanent" magnet mineral.
By "permanent," it is meant that the material maintains a magnetic field with no external
help. The characteristic of any magnetic material to do so is called retentivity.
• Ferromagnetic materials are easily magnetized.
• Paramagnetic materials are magnetized with more difficulty.
• Diamagnetic materials actually tend to repel external magnetic fields by magnetizing in
the opposite direction.
14.2
Electromagnetism
The discovery of the relationship between magnetism and electricity was, like so many other
scientific discoveries, stumbled upon almost by accident. The Danish physicist Hans Christian
Oersted was lecturing one day in 1820 on the possibility of electricity and magnetism being
related to one another, and in the process demonstrated it conclusively by experiment in front
of his whole class! By passing an electric current through a metal wire suspended above a mag-
netic compass, Oersted was able to produce a definite motion of the compass needle in response
to the current. What began as conjecture at the start of the class session was confirmed as fact
at the end. Needless to say, Oersted had to revise his lecture notes for future classes! His
serendipitous discovery paved the way for a whole new branch of science: electromagnetics.
Detailed experiments showed that the magnetic field produced by an electric current is
always oriented perpendicular to the direction of flow. A simple method of showing this rela-
tionship is called the left-hand rule. Simply stated, the left-hand rule says that the magnetic
flux lines produced by a current-carrying wire will be oriented the same direction as the curled
fingers of a person's left hand (in the "hitchhiking" position), with the thumb pointing in the
direction of electron flow:
I
I
I
I
The "left-hand" rule
The magnetic field encircles this straight piece of current-carrying wire, the magnetic flux
lines having no definite "north" or "south' poles.
While the magnetic field surrounding a current-carrying wire is indeed interesting, it is
quite weak for common amounts of current, able to deflect a compass needle and not much
466
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
more. To create a stronger magnetic field force (and consequently, more field flux) with the
same amount of electric current, we can wrap the wire into a coil shape, where the circling
magnetic fields around the wire will join to create a larger field with a definite magnetic (north
and south) polarity:
magnetic field
N
S
The amount of magnetic field force generated by a coiled wire is proportional to the current
through the wire multiplied by the number of "turns" or "wraps" of wire in the coil. This field
force is called magnetomotive force (mmf), and is very much analogous to electromotive force
(E) in an electric circuit.
An electromagnet is a piece of wire intended to generate a magnetic field with the passage of
electric current through it. Though all current-carrying conductors produce magnetic fields, an
electromagnet is usually constructed in such a way as to maximize the strength of the magnetic
field it produces for a special purpose. Electromagnets find frequent application in research,
industry, medical, and consumer products.
As an electrically-controllable magnet, electromagnets find application in a wide variety of
"electromechanical" devices: machines that effect mechanical force or motion through electrical
power. Perhaps the most obvious example of such a machine is the electric motor.
Another example is the relay, an electrically-controlled switch. If a switch contact mecha-
nism is built so that it can be actuated (opened and closed) by the application of a magnetic
field, and an electromagnet coil is placed in the near vicinity to produce that requisite field, it
will be possible to open and close the switch by the application of a current through the coil. In
effect, this gives us a device that enables elelctricity to control electricity:
14.3.
MAGNETIC UNITS OF MEASUREMENT
467
Relay
Applying current through the coil
causes the switch to close.
Relays can be constructed to actuate multiple switch contacts, or operate them in "reverse"
(energizing the coil will open the switch contact, and unpowering the coil will allow it to spring
closed again).
Multiple-contact
relay
Relay with "normally-
closed" contact
• REVIEW:
• When electrons flow through a conductor, a magnetic field will be produced around that
conductor.
• The left-hand rule states that the magnetic flux lines produced by a current-carrying
wire will be oriented the same direction as the curled fingers of a person's left hand (in
the "hitchhiking" position), with the thumb pointing in the direction of electron flow.
• The magnetic field force produced by a current-carrying wire can be greatly increased
by shaping the wire into a coil instead of a straight line. If wound in a coil shape, the
magnetic field will be oriented along the axis of the coil's length.
• The magnetic field force produced by an electromagnet (called the magnetomotive force,
or mmf), is proportional to the product (multiplication) of the current through the elec-
tromagnet and the number of complete coil "turns" formed by the wire.
14.3
Magnetic units of measurement
If the burden of two systems of measurement for common quantities (English vs.
metric)
throws your mind into confusion, this is not the place for you! Due to an early lack of standard-
468
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
ization in the science of magnetism, we have been plagued with no less than three complete
systems of measurement for magnetic quantities.
First, we need to become acquainted with the various quantities associated with mag-
netism. There are quite a few more quantities to be dealt with in magnetic systems than
for electrical systems. With electricity, the basic quantities are Voltage (E), Current (I), Re-
sistance (R), and Power (P). The first three are related to one another by Ohm's Law (E=IR ;
I=E/R ; R=E/I), while Power is related to voltage, current, and resistance by Joule's Law (P=IE
With magnetism, we have the following quantities to deal with:
Magnetomotive Force - The quantity of magnetic field force, or "push." Analogous to
electric voltage (electromotive force).
Field Flux - The quantity of total field effect, or "substance" of the field. Analogous to
electric current.
Field Intensity - The amount of field force (mmf) distributed over the length of the elec-
tromagnet. Sometimes referred to as Magnetizing Force.
Flux Density - The amount of magnetic field flux concentrated in a given area.
Reluctance - The opposition to magnetic field flux through a given volume of space or
material. Analogous to electrical resistance.
Permeability - The specific measure of a material's acceptance of magnetic flux, analogous
to the specific resistance of a conductive material (ρ), except inverse (greater permeability
means easier passage of magnetic flux, whereas greater specific resistance means more difficult
passage of electric current).
But wait . . . the fun is just beginning! Not only do we have more quantities to keep track of
with magnetism than with electricity, but we have several different systems of unit measure-
ment for each of these quantities. As with common quantities of length, weight, volume, and
temperature, we have both English and metric systems. However, there is actually more than
one metric system of units, and multiple metric systems are used in magnetic field measure-
ments! One is called the cgs, which stands for Centimeter-Gram-Second, denoting the root
measures upon which the whole system is based. The other was originally known as the mks
system, which stood for Meter-Kilogram-Second, which was later revised into another system,
called rmks, standing for Rationalized Meter-Kilogram-Second. This ended up being adopted
as an international standard and renamed SI (Systeme International).
14.3.
MAGNETIC UNITS OF MEASUREMENT
469
Quantity
Symbol
Measurement
Unit of
and abbreviation
Field Force
Field Flux
Field
Intensity
Flux
Density
CGS
SI
English
Reluctance
Permeability
mmf
Φ
H
B
μ
Gilbert (Gb)
Amp-turn
Amp-turn
Oersted (Oe)
Maxwell (Mx) Weber (Wb)
Line
Gauss (G)
Tesla (T)
per meter
per inch
Lines per
square inch
Gilberts per
Maxwell
Amp-turns
per Weber
Amp-turns
Amp-turns
Amp-turns
per line
Gauss per
Oersted
Lines per
Tesla-meters
per Amp-turn
turn
R
And yes, the μ symbol is really the same as the metric prefix "micro." I find this especially
confusing, using the exact same alphabetical character to symbolize both a specific quantity
and a general metric prefix!
As you might have guessed already, the relationship between field force, field flux, and
reluctance is much the same as that between the electrical quantities of electromotive force
(E), current (I), and resistance (R). This provides something akin to an Ohm's Law for magnetic
circuits:
Electrical
Magnetic
A comparison of "Ohm's Law" for
electric and magnetic circuits:
And, given that permeability is inversely analogous to specific resistance, the equation for
finding the reluctance of a magnetic material is very similar to that for finding the resistance
of a conductor:
Electrical
Magnetic
l
A
l
μA
A comparison of electrical
and magnetic opposition:
In either case, a longer piece of material provides a greater opposition, all other factors
being equal. Also, a larger cross-sectional area makes for less opposition, all other factors
being equal.
470
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
The major caveat here is that the reluctance of a material to magnetic flux actually changes
with the concentration of flux going through it. This makes the "Ohm's Law" for magnetic
circuits nonlinear and far more difficult to work with than the electrical version of Ohm's Law.
It would be analogous to having a resistor that changed resistance as the current through it
varied (a circuit composed of varistors instead of resistors).
14.4
Permeability and saturation
The nonlinearity of material permeability may be graphed for better understanding. We'll
place the quantity of field intensity (H), equal to field force (mmf) divided by the length of the
material, on the horizontal axis of the graph. On the vertical axis, we'll place the quantity of
flux density (B), equal to total flux divided by the cross-sectional area of the material. We will
use the quantities of field intensity (H) and flux density (B) instead of field force (mmf) and
total flux (Φ) so that the shape of our graph remains independent of the physical dimensions of
our test material. What we're trying to do here is show a mathematical relationship between
field force and flux for any chunk of a particular substance, in the same spirit as describing a
material's specific resistance in ohm-cmil/ft instead of its actual resistance in ohms.
Flux density
(B)
Field intensity (H)
cast iron
cast steel
sheet steel
This is called the normal magnetization curve, or B-H curve, for any particular material.
Notice how the flux density for any of the above materials (cast iron, cast steel, and sheet
steel) levels off with increasing amounts of field intensity. This effect is known as saturation.
When there is little applied magnetic force (low H), only a few atoms are in alignment, and
the rest are easily aligned with additional force. However, as more flux gets crammed into
the same cross-sectional area of a ferromagnetic material, fewer atoms are available within
that material to align their electrons with additional force, and so it takes more and more
force (H) to get less and less "help" from the material in creating more flux density (B). To
put this in economic terms, we're seeing a case of diminishing returns (B) on our investment
(H). Saturation is a phenomenon limited to iron-core electromagnets. Air-core electromagnets
don't saturate, but on the other hand they don't produce nearly as much magnetic flux as a
ferromagnetic core for the same number of wire turns and current.
Another quirk to confound our analysis of magnetic flux versus force is the phenomenon
of magnetic hysteresis. As a general term, hysteresis means a lag between input and output
14.4.
PERMEABILITY AND SATURATION
471
in a system upon a change in direction. Anyone who's ever driven an old automobile with
"loose" steering knows what hysteresis is: to change from turning left to turning right (or vice
versa), you have to rotate the steering wheel an additional amount to overcome the built-in
"lag" in the mechanical linkage system between the steering wheel and the front wheels of the
car. In a magnetic system, hysteresis is seen in a ferromagnetic material that tends to stay
magnetized after an applied field force has been removed (see "retentivity" in the first section
of this chapter), if the force is reversed in polarity.
Let's use the same graph again, only extending the axes to indicate both positive and neg-
ative quantities. First we'll apply an increasing field force (current through the coils of our
electromagnet). We should see the flux density increase (go up and to the right) according to
the normal magnetization curve:
Flux density
(B)
Field intensity (H)
Next, we'll stop the current going through the coil of the electromagnet and see what hap-
pens to the flux, leaving the first curve still on the graph:
Flux density
(B)
Field intensity (H)
472
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
Due to the retentivity of the material, we still have a magnetic flux with no applied force
(no current through the coil). Our electromagnet core is acting as a permanent magnet at
this point. Now we will slowly apply the same amount of magnetic field force in the opposite
direction to our sample:
Flux density
(B)
Field intensity (H)
The flux density has now reached a point equivalent to what it was with a full positive value
of field intensity (H), except in the negative, or opposite, direction. Let's stop the current going
through the coil again and see how much flux remains:
Flux density
(B)
Field intensity (H)
Once again, due to the natural retentivity of the material, it will hold a magnetic flux with
no power applied to the coil, except this time its in a direction opposite to that of the last time
we stopped current through the coil. If we re-apply power in a positive direction again, we
should see the flux density reach its prior peak in the upper-right corner of the graph again:
14.4.
PERMEABILITY AND SATURATION
473
Flux density
(B)
Field intensity (H)
The "S"-shaped curve traced by these steps form what is called the hysteresis curve of a
ferromagnetic material for a given set of field intensity extremes (-H and +H). If this doesn't
quite make sense, consider a hysteresis graph for the automobile steering scenario described
earlier, one graph depicting a "tight" steering system and one depicting a "loose" system:
angle of front wheels
rotation of
steering wheel
(right)
(left)
(CCW)
(CW)
An ideal steering system
474
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
angle of front wheels
rotation of
steering wheel
(right)
(left)
(CCW)
(CW)
amount of "looseness"
in the steering mechanism
A "loose" steering system
Just as in the case of automobile steering systems, hysteresis can be a problem. If you're
designing a system to produce precise amounts of magnetic field flux for given amounts of
current, hysteresis may hinder this design goal (due to the fact that the amount of flux density
would depend on the current and how strongly it was magnetized before!). Similarly, a loose
steering system is unacceptable in a race car, where precise, repeatable steering response is
a necessity. Also, having to overcome prior magnetization in an electromagnet can be a waste
of energy if the current used to energize the coil is alternating back and forth (AC). The area
within the hysteresis curve gives a rough estimate of the amount of this wasted energy.
Other times, magnetic hysteresis is a desirable thing. Such is the case when magnetic ma-
terials are used as a means of storing information (computer disks, audio and video tapes). In
these applications, it is desirable to be able to magnetize a speck of iron oxide (ferrite) and rely
on that material's retentivity to "remember" its last magnetized state. Another productive ap-
plication for magnetic hysteresis is in filtering high-frequency electromagnetic "noise" (rapidly
alternating surges of voltage) from signal wiring by running those wires through the middle
of a ferrite ring. The energy consumed in overcoming the hysteresis of ferrite attenuates the
strength of the "noise" signal. Interestingly enough, the hysteresis curve of ferrite is quite
extreme:
14.5.
ELECTROMAGNETIC INDUCTION
475
Field intensity (H)
Flux density
(B)
Hysteresis curve for ferrite
• REVIEW:
• The permeability of a material changes with the amount of magnetic flux forced through
it.
• The specific relationship of force to flux (field intensity H to flux density B) is graphed in
a form called the normal magnetization curve.
• It is possible to apply so much magnetic field force to a ferromagnetic material that no
more flux can be crammed into it. This condition is known as magnetic saturation.
• When the retentivity of a ferromagnetic substance interferes with its re-magnetization in
the opposite direction, a condition known as hysteresis occurs.
14.5
Electromagnetic induction
While Oersted's surprising discovery of electromagnetism paved the way for more practical ap-
plications of electricity, it was Michael Faraday who gave us the key to the practical generation
of electricity: electromagnetic induction. Faraday discovered that a voltage would be gener-
ated across a length of wire if that wire was exposed to a perpendicular magnetic field flux of
changing intensity.
An easy way to create a magnetic field of changing intensity is to move a permanent magnet
next to a wire or coil of wire. Remember: the magnetic field must increase or decrease in
intensity perpendicular to the wire (so that the lines of flux "cut across" the conductor), or else
no voltage will be induced:
476
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
V
magnet moved
back and forth
N
S
voltage changes polarity
with change in magnet motion
with change in magnet motion
current changes direction
Electromagnetic induction
Faraday was able to mathematically relate the rate of change of the magnetic field flux with
induced voltage (note the use of a lower-case letter "e" for voltage. This refers to instantaneous
voltage, or voltage at a specific point in time, rather than a steady, stable voltage.):
dΦ
dt
Where,
N
Number of turns in wire coil (straight wire = 1)
Magnetic flux in Webers
Time in seconds
e = (Instantaneous) induced voltage in volts
The "d" terms are standard calculus notation, representing rate-of-change of flux over time.
"N" stands for the number of turns, or wraps, in the wire coil (assuming that the wire is formed
in the shape of a coil for maximum electromagnetic efficiency).
This phenomenon is put into obvious practical use in the construction of electrical gen-
erators, which use mechanical power to move a magnetic field past coils of wire to generate
voltage. However, this is by no means the only practical use for this principle.
If we recall that the magnetic field produced by a current-carrying wire was always perpen-
dicular to that wire, and that the flux intensity of that magnetic field varied with the amount
of current through it, we can see that a wire is capable of inducing a voltage along its own
length simply due to a change in current through it. This effect is called self-induction: a
changing magnetic field produced by changes in current through a wire inducing voltage along
the length of that same wire. If the magnetic field flux is enhanced by bending the wire into the
shape of a coil, and/or wrapping that coil around a material of high permeability, this effect of
14.6.
MUTUAL INDUCTANCE
477
self-induced voltage will be more intense. A device constructed to take advantage of this effect
is called an inductor, and will be discussed in greater detail in the next chapter.
• REVIEW:
• A magnetic field of changing intensity perpendicular to a wire will induce a voltage along
the length of that wire. The amount of voltage induced depends on the rate of change of
the magnetic field flux and the number of turns of wire (if coiled) exposed to the change
in flux.
• Faraday's equation for induced voltage: e = N(dΦ/dt)
• A current-carrying wire will experience an induced voltage along its length if the current
changes (thus changing the magnetic field flux perpendicular to the wire, thus inducing
voltage according to Faraday's formula). A device built specifically to take advantage of
this effect is called an inductor.
14.6
Mutual inductance
If two coils of wire are brought into close proximity with each other so the magnetic field from
one links with the other, a voltage will be generated in the second coil as a result. This is called
mutual inductance: when voltage impressed upon one coil induces a voltage in another.
A device specifically designed to produce the effect of mutual inductance between two or
more coils is called a transformer.
478
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
The device shown in the above photograph is a kind of transformer, with two concentric
wire coils. It is actually intended as a precision standard unit for mutual inductance, but for
the purposes of illustrating what the essence of a transformer is, it will suffice. The two wire
coils can be distinguished from each other by color: the bulk of the tube's length is wrapped in
green-insulated wire (the first coil) while the second coil (wire with bronze-colored insulation)
stands in the middle of the tube's length. The wire ends run down to connection terminals at
the bottom of the unit. Most transformer units are not built with their wire coils exposed like
this.
Because magnetically-induced voltage only happens when the magnetic field flux is chang-
ing in strength relative to the wire, mutual inductance between two coils can only happen with
alternating (changing - AC) voltage, and not with direct (steady - DC) voltage. The only ap-
plications for mutual inductance in a DC system is where some means is available to switch
power on and off to the coil (thus creating a pulsing DC voltage), the induced voltage peaking
at every pulse.
A very useful property of transformers is the ability to transform voltage and current levels
according to a simple ratio, determined by the ratio of input and output coil turns. If the
energized coil of a transformer is energized by an AC voltage, the amount of AC voltage induced
in the unpowered coil will be equal to the input voltage multiplied by the ratio of output to
input wire turns in the coils. Conversely, the current through the windings of the output coil
compared to the input coil will follow the opposite ratio: if the voltage is increased from input
coil to output coil, the current will be decreased by the same proportion. This action of the
transformer is analogous to that of mechanical gear, belt sheave, or chain sprocket ratios:
14.7.
CONTRIBUTORS
479
Large gear
Small gear
(many teeth)
(few teeth)
high torque, low speed
low torque, high speed
AC voltage
source
Load
high voltage
low current
low voltage
high current
many
turns
few turns
Torque-reducing geartrain
"Step-down" transformer
A transformer designed to output more voltage than it takes in across the input coil is called
a "step-up" transformer, while one designed to do the opposite is called a "step-down," in ref-
erence to the transformation of voltage that takes place. The current through each respective
coil, of course, follows the exact opposite proportion.
• REVIEW:
• Mutual inductance is where the magnetic field generated by a coil of wire induces voltage
in an adjacent coil of wire.
• A transformer is a device constructed of two or more coils in close proximity to each other,
with the express purpose of creating a condition of mutual inductance between the coils.
• Transformers only work with changing voltages, not steady voltages. Thus, they may be
classified as an AC device and not a DC device.
14.7
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
480
CHAPTER 14. MAGNETISM AND ELECTROMAGNETISM
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 15
INDUCTORS
Contents
15.1 Magnetic fields and inductance . . . . . . . . . . . . . . . . . . . . . . . . . . 481
15.2 Inductors and calculus
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 485
15.3 Factors affecting inductance . . . . . . . . . . . . . . . . . . . . . . . . . . . . 491
15.4 Series and parallel inductors
. . . . . . . . . . . . . . . . . . . . . . . . . . . 496
15.5 Practical considerations
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 498
15.6 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 498
15.1
Magnetic fields and inductance
Whenever electrons flow through a conductor, a magnetic field will develop around that con-
ductor. This effect is called electromagnetism. Magnetic fields affect the alignment of electrons
in an atom, and can cause physical force to develop between atoms across space just as with
electric fields developing force between electrically charged particles. Like electric fields, mag-
netic fields can occupy completely empty space, and affect matter at a distance.
Fields have two measures: a field force and a field flux. The field force is the amount of
"push" that a field exerts over a certain distance. The field flux is the total quantity, or effect,
of the field through space. Field force and flux are roughly analogous to voltage ("push") and
current (flow) through a conductor, respectively, although field flux can exist in totally empty
space (without the motion of particles such as electrons) whereas current can only take place
where there are free electrons to move. Field flux can be opposed in space, just as the flow
of electrons can be opposed by resistance. The amount of field flux that will develop in space
is proportional to the amount of field force applied, divided by the amount of opposition to
flux. Just as the type of conducting material dictates that conductor's specific resistance to
electric current, the type of material occupying the space through which a magnetic field force
is impressed dictates the specific opposition to magnetic field flux.
481
482
CHAPTER 15. INDUCTORS
Whereas an electric field flux between two conductors allows for an accumulation of free
electron charge within those conductors, a magnetic field flux allows for a certain "inertia" to
accumulate in the flow of electrons through the conductor producing the field.
Inductors are components designed to take advantage of this phenomenon by shaping the
length of conductive wire in the form of a coil. This shape creates a stronger magnetic field
than what would be produced by a straight wire. Some inductors are formed with wire wound
in a self-supporting coil. Others wrap the wire around a solid core material of some type.
Sometimes the core of an inductor will be straight, and other times it will be joined in a loop
(square, rectangular, or circular) to fully contain the magnetic flux. These design options all
have an effect on the performance and characteristics of inductors.
The schematic symbol for an inductor, like the capacitor, is quite simple, being little more
than a coil symbol representing the coiled wire. Although a simple coil shape is the generic
symbol for any inductor, inductors with cores are sometimes distinguished by the addition of
parallel lines to the axis of the coil. A newer version of the inductor symbol dispenses with the
coil shape in favor of several "humps" in a row:
generic, or air-core
iron core
iron core
(alternative)
generic
(newer symbol)
Inductor symbols
As the electric current produces a concentrated magnetic field around the coil, this field flux
equates to a storage of energy representing the kinetic motion of the electrons through the coil.
The more current in the coil, the stronger the magnetic field will be, and the more energy the
inductor will store.
I
I
magnetic
field
Because inductors store the kinetic energy of moving electrons in the form of a magnetic
field, they behave quite differently than resistors (which simply dissipate energy in the form of
heat) in a circuit. Energy storage in an inductor is a function of the amount of current through
it. An inductor's ability to store energy as a function of current results in a tendency to try
15.1.
MAGNETIC FIELDS AND INDUCTANCE
483
to maintain current at a constant level. In other words, inductors tend to resist changes in
current. When current through an inductor is increased or decreased, the inductor "resists"
the change by producing a voltage between its leads in opposing polarity to the change.
To store more energy in an inductor, the current through it must be increased. This means
that its magnetic field must increase in strength, and that change in field strength produces the
corresponding voltage according to the principle of electromagnetic self-induction. Conversely,
to release energy from an inductor, the current through it must be decreased. This means
that the inductor's magnetic field must decrease in strength, and that change in field strength
self-induces a voltage drop of just the opposite polarity.
Just as Isaac Newton's first Law of Motion ("an object in motion tends to stay in motion;
an object at rest tends to stay at rest") describes the tendency of a mass to oppose changes in
velocity, we can state an inductor's tendency to oppose changes in current as such: "Electrons
moving through an inductor tend to stay in motion; electrons at rest in an inductor tend to
stay at rest." Hypothetically, an inductor left short-circuited will maintain a constant rate of
current through it with no external assistance:
current sustained with
the inductor short-circuited
Practically speaking, however, the ability for an inductor to self-sustain current is realized
only with superconductive wire, as the wire resistance in any normal inductor is enough to
cause current to decay very quickly with no external source of power.
When the current through an inductor is increased, it drops a voltage opposing the direction
of electron flow, acting as a power load. In this condition the inductor is said to be charging,
because there is an increasing amount of energy being stored in its magnetic field. Note the
polarity of the voltage with regard to the direction of current:
. . . to the rest of
the circuit
Energy being absorbed by
the inductor from the rest
increasing current
increasing current
voltage drop
The inductor acts as a LOAD
of the circuit.
Conversely, when the current through the inductor is decreased, it drops a voltage aiding
484
CHAPTER 15. INDUCTORS
the direction of electron flow, acting as a power source. In this condition the inductor is said to
be discharging, because its store of energy is decreasing as it releases energy from its magnetic
field to the rest of the circuit. Note the polarity of the voltage with regard to the direction of
current.
. . . to the rest of
the circuit
voltage drop
Energy being released by
the inductor to the rest
decreasing current
decreasing current
The inductor acts as a SOURCE
of the circuit.
If a source of electric power is suddenly applied to an unmagnetized inductor, the inductor
will initially resist the flow of electrons by dropping the full voltage of the source. As current
begins to increase, a stronger and stronger magnetic field will be created, absorbing energy
from the source. Eventually the current reaches a maximum level, and stops increasing. At
this point, the inductor stops absorbing energy from the source, and is dropping minimum
voltage across its leads, while the current remains at a maximum level. As an inductor stores
more energy, its current level increases, while its voltage drop decreases. Note that this is
precisely the opposite of capacitor behavior, where the storage of energy results in an increased
voltage across the component! Whereas capacitors store their energy charge by maintaining
a static voltage, inductors maintain their energy "charge" by maintaining a steady current
through the coil.
The type of material the wire is coiled around greatly impacts the strength of the mag-
netic field flux (and therefore the amount of stored energy) generated for any given amount
of current through the coil. Coil cores made of ferromagnetic materials (such as soft iron) will
encourage stronger field fluxes to develop with a given field force than nonmagnetic substances
such as aluminum or air.
The measure of an inductor's ability to store energy for a given amount of current flow is
called inductance. Not surprisingly, inductance is also a measure of the intensity of opposition
to changes in current (exactly how much self-induced voltage will be produced for a given rate
of change of current). Inductance is symbolically denoted with a capital "L," and is measured
in the unit of the Henry, abbreviated as "H."
An obsolete name for an inductor is choke, so called for its common usage to block ("choke")
high-frequency AC signals in radio circuits. Another name for an inductor, still used in modern
times, is reactor, especially when used in large power applications. Both of these names will
make more sense after you've studied alternating current (AC) circuit theory, and especially a
principle known as inductive reactance.
• REVIEW:
15.2.
INDUCTORS AND CALCULUS
485
• Inductors react against changes in current by dropping voltage in the polarity necessary
to oppose the change.
• When an inductor is faced with an increasing current, it acts as a load: dropping voltage
as it absorbs energy (negative on the current entry side and positive on the current exit
side, like a resistor).
• When an inductor is faced with a decreasing current, it acts as a source: creating voltage
as it releases stored energy (positive on the current entry side and negative on the current
exit side, like a battery).
• The ability of an inductor to store energy in the form of a magnetic field (and consequently
to oppose changes in current) is called inductance. It is measured in the unit of the Henry
(H).
• Inductors used to be commonly known by another term: choke. In large power applica-
tions, they are sometimes referred to as reactors.
15.2
Inductors and calculus
Inductors do not have a stable "resistance" as conductors do.
However, there is a definite
mathematical relationship between voltage and current for an inductor, as follows:
dt
Where,
dt
"Ohm's Law" for an inductor
di
L
v = Instantaneous voltage across the inductor
L = Inductance in Henrys
di
= Instantaneous rate of current change
(amps per second)
You should recognize the form of this equation from the capacitor chapter. It relates one
variable (in this case, inductor voltage drop) to a rate of change of another variable (in this
case, inductor current). Both voltage (v) and rate of current change (di/dt) are instantaneous:
that is, in relation to a specific point in time, thus the lower-case letters "v" and "i". As with
the capacitor formula, it is convention to express instantaneous voltage as v rather than e, but
using the latter designation would not be wrong. Current rate-of-change (di/dt) is expressed in
units of amps per second, a positive number representing an increase and a negative number
representing a decrease.
Like a capacitor, an inductor's behavior is rooted in the variable of time. Aside from any
resistance intrinsic to an inductor's wire coil (which we will assume is zero for the sake of
486
CHAPTER 15. INDUCTORS
this section), the voltage dropped across the terminals of an inductor is purely related to how
quickly its current changes over time.
Suppose we were to connect a perfect inductor (one having zero ohms of wire resistance) to
a circuit where we could vary the amount of current through it with a potentiometer connected
as a variable resistor:
(zero-center)
A
Voltmeter
If the potentiometer mechanism remains in a single position (wiper is stationary), the
series-connected ammeter will register a constant (unchanging) current, and the voltmeter
connected across the inductor will register 0 volts. In this scenario, the instantaneous rate of
current change (di/dt) is equal to zero, because the current is stable. The equation tells us that
with 0 amps per second change for a di/dt, there must be zero instantaneous voltage (v) across
the inductor. From a physical perspective, with no current change, there will be a steady mag-
netic field generated by the inductor. With no change in magnetic flux (dΦ/dt = 0 Webers per
second), there will be no voltage dropped across the length of the coil due to induction.
15.2.
INDUCTORS AND CALCULUS
487
Time
Time
Potentiometer wiper not moving
Inductor
current
IL
Inductor
voltage
EL
If we move the potentiometer wiper slowly in the "up" direction, its resistance from end to
end will slowly decrease. This has the effect of increasing current in the circuit, so the ammeter
indication should be increasing at a slow rate:
A
Potentiometer wiper moving
slowly in the "up" direction
Increasing
current
Steady
voltage
Assuming that the potentiometer wiper is being moved such that the rate of current in-
crease through the inductor is steady, the di/dt term of the formula will be a fixed value. This
fixed value, multiplied by the inductor's inductance in Henrys (also fixed), results in a fixed
voltage of some magnitude. From a physical perspective, the gradual increase in current re-
sults in a magnetic field that is likewise increasing. This gradual increase in magnetic flux
causes a voltage to be induced in the coil as expressed by Michael Faraday's induction equa-
488
CHAPTER 15. INDUCTORS
tion e = N(dΦ/dt). This self-induced voltage across the coil, as a result of a gradual change in
current magnitude through the coil, happens to be of a polarity that attempts to oppose the
change in current. In other words, the induced voltage polarity resulting from an increase in
current will be oriented in such a way as to push against the direction of current, to try to keep
the current at its former magnitude. This phenomenon exhibits a more general principle of
physics known as Lenz's Law, which states that an induced effect will always be opposed to the
cause producing it.
In this scenario, the inductor will be acting as a load, with the negative side of the induced
voltage on the end where electrons are entering, and the positive side of the induced voltage
on the end where electrons are exiting.
Time
Time
change
Time
Potentiometer wiper moving slowly "up"
Inductor
current
IL
Current
Inductor
voltage
EL
Changing the rate of current increase through the inductor by moving the potentiometer
wiper "up" at different speeds results in different amounts of voltage being dropped across the
inductor, all with the same polarity (opposing the increase in current):
15.2.
INDUCTORS AND CALCULUS
489
Time
Time
Inductor
current
IL
Inductor
voltage
EL
Potentiometer wiper moving "up" at
different rates
Here again we see the derivative function of calculus exhibited in the behavior of an in-
ductor. In calculus terms, we would say that the induced voltage across the inductor is the
derivative of the current through the inductor: that is, proportional to the current's rate-of-
change with respect to time.
Reversing the direction of wiper motion on the potentiometer (going "down" rather than
"up") will result in its end-to-end resistance increasing.
This will result in circuit current
decreasing (a negative figure for di/dt). The inductor, always opposing any change in current,
will produce a voltage drop opposed to the direction of change:
A
Potentiometer wiper moving
current
in the "down" direction
Decreasing
How much voltage the inductor will produce depends, of course, on how rapidly the current
490
CHAPTER 15. INDUCTORS
through it is decreased. As described by Lenz's Law, the induced voltage will be opposed to the
change in current. With a decreasing current, the voltage polarity will be oriented so as to try
to keep the current at its former magnitude. In this scenario, the inductor will be acting as a
source, with the negative side of the induced voltage on the end where electrons are exiting,
and the positive side of the induced voltage on the end where electrons are entering. The more
rapidly current is decreased, the more voltage will be produced by the inductor, in its release
of stored energy to try to keep current constant.
Again, the amount of voltage across a perfect inductor is directly proportional to the rate
of current change through it. The only difference between the effects of a decreasing current
and an increasing current is the polarity of the induced voltage. For the same rate of current
change over time, either increasing or decreasing, the voltage magnitude (volts) will be the
same. For example, a di/dt of -2 amps per second will produce the same amount of induced
voltage drop across an inductor as a di/dt of +2 amps per second, just in the opposite polarity.
If current through an inductor is forced to change very rapidly, very high voltages will be
produced. Consider the following circuit:
Neon lamp
6 V
Switch
In this circuit, a lamp is connected across the terminals of an inductor. A switch is used to
control current in the circuit, and power is supplied by a 6 volt battery. When the switch is
closed, the inductor will briefly oppose the change in current from zero to some magnitude, but
will drop only a small amount of voltage. It takes about 70 volts to ionize the neon gas inside a
neon bulb like this, so the bulb cannot be lit on the 6 volts produced by the battery, or the low
voltage momentarily dropped by the inductor when the switch is closed:
6 V
no light
15.3.
FACTORS AFFECTING INDUCTANCE
491
When the switch is opened, however, it suddenly introduces an extremely high resistance
into the circuit (the resistance of the air gap between the contacts). This sudden introduction
of high resistance into the circuit causes the circuit current to decrease almost instantly. Math-
ematically, the di/dt term will be a very large negative number. Such a rapid change of current
(from some magnitude to zero in very little time) will induce a very high voltage across the
inductor, oriented with negative on the left and positive on the right, in an effort to oppose this
decrease in current. The voltage produced is usually more than enough to light the neon lamp,
if only for a brief moment until the current decays to zero:
6 V
Light!
For maximum effect, the inductor should be sized as large as possible (at least 1 Henry of
inductance).
15.3
Factors affecting inductance
There are four basic factors of inductor construction determining the amount of inductance
created. These factors all dictate inductance by affecting how much magnetic field flux will
develop for a given amount of magnetic field force (current through the inductor's wire coil):
NUMBER OF WIRE WRAPS, OR "TURNS" IN THE COIL: All other factors being
equal, a greater number of turns of wire in the coil results in greater inductance; fewer turns
of wire in the coil results in less inductance.
Explanation: More turns of wire means that the coil will generate a greater amount of
magnetic field force (measured in amp-turns!), for a given amount of coil current.
less inductance
more inductance
COIL AREA: All other factors being equal, greater coil area (as measured looking length-
wise through the coil, at the cross-section of the core) results in greater inductance; less coil
area results in less inductance.
492
CHAPTER 15. INDUCTORS
Explanation: Greater coil area presents less opposition to the formation of magnetic field
flux, for a given amount of field force (amp-turns).
less inductance
more inductance
COIL LENGTH: All other factors being equal, the longer the coil's length, the less induc-
tance; the shorter the coil's length, the greater the inductance.
Explanation: A longer path for the magnetic field flux to take results in more opposition to
the formation of that flux for any given amount of field force (amp-turns).
less inductance
more inductance
CORE MATERIAL: All other factors being equal, the greater the magnetic permeability of
the core which the coil is wrapped around, the greater the inductance; the less the permeability
of the core, the less the inductance.
Explanation: A core material with greater magnetic permeability results in greater mag-
netic field flux for any given amount of field force (amp-turns).
less inductance
more inductance
air core
(permeability = 1)
soft iron core
(permeability = 600)
An approximation of inductance for any coil of wire can be found with this formula:
15.3.
FACTORS AFFECTING INDUCTANCE
493
Where,
N = Number of turns in wire coil (straight wire = 1)
l
Inductance of coil in Henrys
Permeability of core material (absolute, not relative)
Area of coil in square meters = πr2
Average length of coil in meters
Relative permeability, dimensionless (μ0=1 for air)
1.26 x 10 -6 T-m/At permeability of free space
r
l
It must be understood that this formula yields approximate figures only. One reason for
this is the fact that permeability changes as the field intensity varies (remember the nonlinear
"B/H" curves for different materials). Obviously, if permeability (μ) in the equation is unstable,
then the inductance (L) will also be unstable to some degree as the current through the coil
changes in magnitude. If the hysteresis of the core material is significant, this will also have
strange effects on the inductance of the coil. Inductor designers try to minimize these effects
by designing the core in such a way that its flux density never approaches saturation levels,
and so the inductor operates in a more linear portion of the B/H curve.
If an inductor is designed so that any one of these factors may be varied at will, its induc-
tance will correspondingly vary. Variable inductors are usually made by providing a way to
vary the number of wire turns in use at any given time, or by varying the core material (a slid-
ing core that can be moved in and out of the coil). An example of the former design is shown in
this photograph:
This unit uses sliding copper contacts to tap into the coil at different points along its length.
494
CHAPTER 15. INDUCTORS
The unit shown happens to be an air-core inductor used in early radio work.
A fixed-value inductor is shown in the next photograph, another antique air-core unit built
for radios. The connection terminals can be seen at the bottom, as well as the few turns of
relatively thick wire:
Here is another inductor (of greater inductance value), also intended for radio applications.
Its wire coil is wound around a white ceramic tube for greater rigidity:
Inductors can also be made very small for printed circuit board applications. Closely exam-
ine the following photograph and see if you can identify two inductors near each other:
15.3.
FACTORS AFFECTING INDUCTANCE
495
The two inductors on this circuit board are labeled L1 and L2, and they are located to the
right-center of the board. Two nearby components are R3 (a resistor) and C16 (a capacitor).
These inductors are called "toroidal" because their wire coils are wound around donut-shaped
("torus") cores.
Like resistors and capacitors, inductors can be packaged as "surface mount devices" as well.
The following photograph shows just how small an inductor can be when packaged as such:
496
CHAPTER 15. INDUCTORS
A pair of inductors can be seen on this circuit board, to the right and center, appearing
as small black chips with the number "100" printed on both. The upper inductor's label can
be seen printed on the green circuit board as L5. Of course these inductors are very small in
inductance value, but it demonstrates just how tiny they can be manufactured to meet certain
circuit design needs.
15.4
Series and parallel inductors
When inductors are connected in series, the total inductance is the sum of the individual induc-
tors' inductances. To understand why this is so, consider the following: the definitive measure
of inductance is the amount of voltage dropped across an inductor for a given rate of current
change through it. If inductors are connected together in series (thus sharing the same current,
and seeing the same rate of change in current), then the total voltage dropped as the result of
a change in current will be additive with each inductor, creating a greater total voltage than
either of the individual inductors alone. Greater voltage for the same rate of change in current
means greater inductance.
15.4.
SERIES AND PARALLEL INDUCTORS
497
increase in current
L1
L2
voltage
drop
total voltage drop
voltage
drop
Thus, the total inductance for series inductors is more than any one of the individual in-
ductors' inductances. The formula for calculating the series total inductance is the same form
as for calculating series resistances:
Series Inductances
When inductors are connected in parallel, the total inductance is less than any one of the
parallel inductors' inductances. Again, remember that the definitive measure of inductance is
the amount of voltage dropped across an inductor for a given rate of current change through
it. Since the current through each parallel inductor will be a fraction of the total current, and
the voltage across each parallel inductor will be equal, a change in total current will result
in less voltage dropped across the parallel array than for any one of the inductors considered
separately. In other words, there will be less voltage dropped across parallel inductors for a
given rate of change in current than for any of those inductors considered separately, because
total current divides among parallel branches. Less voltage for the same rate of change in
current means less inductance.
increase in current
L1
L2
voltage
drop
IL1
IL2
total
Thus, the total inductance is less than any one of the individual inductors' inductances.
The formula for calculating the parallel total inductance is the same form as for calculating
parallel resistances:
1
1
Parallel Inductances
L1
L2
Ln
• REVIEW:
• Inductances add in series.
• Inductances diminish in parallel.
498
CHAPTER 15. INDUCTORS
15.5
Practical considerations
Inductors, like all electrical components, have limitations which must be respected for the sake
of reliability and proper circuit operation.
Rated current: Since inductors are constructed of coiled wire, and any wire will be limited
in its current-carrying capacity by its resistance and ability to dissipate heat, you must pay
attention to the maximum current allowed through an inductor.
Equivalent circuit: Since inductor wire has some resistance, and circuit design constraints
typically demand the inductor be built to the smallest possible dimensions, there is no such
thing as a "perfect" inductor.
Inductor coil wire usually presents a substantial amount of
series resistance, and the close spacing of wire from one coil turn to another (separated by
insulation) may present measurable amounts of stray capacitance to interact with its purely
inductive characteristics. Unlike capacitors, which are relatively easy to manufacture with
negligible stray effects, inductors are difficult to find in "pure" form. In certain applications,
these undesirable characteristics may present significant engineering problems.
Inductor size: Inductors tend to be much larger, physically, than capacitors are for storing
equivalent amounts of energy. This is especially true considering the recent advances in elec-
trolytic capacitor technology, allowing incredibly large capacitance values to be packed into a
small package. If a circuit designer needs to store a large amount of energy in a small vol-
ume and has the freedom to choose either capacitors or inductors for the task, he or she will
most likely choose a capacitor. A notable exception to this rule is in applications requiring
huge amounts of either capacitance or inductance to store electrical energy: inductors made
of superconducting wire (zero resistance) are more practical to build and safely operate than
capacitors of equivalent value, and are probably smaller too.
Interference: Inductors may affect nearby components on a circuit board with their magnetic
fields, which can extend significant distances beyond the inductor. This is especially true if
there are other inductors nearby on the circuit board. If the magnetic fields of two or more
inductors are able to "link" with each others' turns of wire, there will be mutual inductance
present in the circuit as well as self-inductance, which could very well cause unwanted effects.
This is another reason why circuit designers tend to choose capacitors over inductors to perform
similar tasks: capacitors inherently contain their respective electric fields neatly within the
component package and therefore do not typically generate any "mutual" effects with other
components.
15.6
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Chapter 16
RC AND L/R TIME CONSTANTS
Contents
16.1 Electrical transients . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 499
16.2 Capacitor transient response
. . . . . . . . . . . . . . . . . . . . . . . . . . . 499
16.3 Inductor transient response . . . . . . . . . . . . . . . . . . . . . . . . . . . . 502
16.4 Voltage and current calculations . . . . . . . . . . . . . . . . . . . . . . . . . 505
16.5 Why L/R and not LR?
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 511
16.6 Complex voltage and current calculations . . . . . . . . . . . . . . . . . . . 514
16.7 Complex circuits
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 515
16.8 Solving for unknown time
. . . . . . . . . . . . . . . . . . . . . . . . . . . . . 520
16.9 Contributors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 522
16.1
Electrical transients
This chapter explores the response of capacitors and inductors to sudden changes in DC volt-
age (called a transient voltage), when wired in series with a resistor. Unlike resistors, which
respond instantaneously to applied voltage, capacitors and inductors react over time as they
absorb and release energy.
16.2
Capacitor transient response
Because capacitors store energy in the form of an electric field, they tend to act like small
secondary-cell batteries, being able to store and release electrical energy. A fully discharged
capacitor maintains zero volts across its terminals, and a charged capacitor maintains a steady
quantity of voltage across its terminals, just like a battery. When capacitors are placed in a
circuit with other sources of voltage, they will absorb energy from those sources, just as a
secondary-cell battery will become charged as a result of being connected to a generator. A fully
discharged capacitor, having a terminal voltage of zero, will initially act as a short-circuit when
499
500
CHAPTER 16. RC AND L/R TIME CONSTANTS
attached to a source of voltage, drawing maximum current as it begins to build a charge. Over
time, the capacitor's terminal voltage rises to meet the applied voltage from the source, and
the current through the capacitor decreases correspondingly. Once the capacitor has reached
the full voltage of the source, it will stop drawing current from it, and behave essentially as an
open-circuit.
Switch
10 kΩ
100 μF
15 V
R
C
When the switch is first closed, the voltage across the capacitor (which we were told was
fully discharged) is zero volts; thus, it first behaves as though it were a short-circuit. Over
time, the capacitor voltage will rise to equal battery voltage, ending in a condition where the
capacitor behaves as an open-circuit. Current through the circuit is determined by the dif-
ference in voltage between the battery and the capacitor, divided by the resistance of 10 kΩ.
As the capacitor voltage approaches the battery voltage, the current approaches zero. Once
the capacitor voltage has reached 15 volts, the current will be exactly zero. Let's see how this
works using real values:
0
Time (seconds)
1
2
3
4
5
6
7
8
9
10
0
2
4
6
8
10
12
14
16
Capacitor voltage
Time
| Battery | Capacitor | Current
voltage
0
15 V
0 V
16.2.
CAPACITOR TRANSIENT RESPONSE
501
0.5
15 V
5.902 V
1
15 V
9.482 V
2
15 V
3
15 V
4
15 V
5
15 V
6
15 V
10
15 V
The capacitor voltage's approach to 15 volts and the current's approach to zero over time
is what a mathematician would call asymptotic: that is, they both approach their final values,
getting closer and closer over time, but never exactly reaches their destinations. For all practi-
cal purposes, though, we can say that the capacitor voltage will eventually reach 15 volts and
that the current will eventually equal zero.
Using the SPICE circuit analysis program, we can chart this asymptotic buildup of capaci-
tor voltage and decay of capacitor current in a more graphical form (capacitor current is plotted
in terms of voltage drop across the resistor, using the resistor as a shunt to measure current):
capacitor charging
v1 1 0 dc 15
r1 1 2 10k
c1 2 0 100u ic=0
.tran .5 10 uic
.plot tran v(2,0) v(1,2)
.end
legend:
Capacitor voltage
+: v(1,2) Capacitor current
time
v(2)
0.000E+00
5.000E+00
1.000E+01
1.500E+01
0.000E+00
5.000E-01
5.881E+00 .
1.000E+00
9.474E+00 .
1.500E+00
1.166E+01 .
2.000E+00
1.297E+01 .
2.500E+00
1.377E+01 .
502
CHAPTER 16. RC AND L/R TIME CONSTANTS
3.000E+00
3.500E+00
4.000E+00
4.500E+00
5.000E+00
5.500E+00
6.000E+00
6.500E+00
7.000E+00
7.500E+00
8.000E+00
8.500E+00
9.000E+00
9.500E+00
1.000E+01
As you can see, I have used the .plot command in the netlist instead of the more familiar
.print command. This generates a pseudo-graphic plot of figures on the computer screen us-
ing text characters. SPICE plots graphs in such a way that time is on the vertical axis (going
down) and amplitude (voltage/current) is plotted on the horizontal (right=more; left=less). No-
tice how the voltage increases (to the right of the plot) very quickly at first, then tapering off
as time goes on. Current also changes very quickly at first then levels off as time goes on, but
it is approaching minimum (left of scale) while voltage approaches maximum.
• REVIEW:
• Capacitors act somewhat like secondary-cell batteries when faced with a sudden change
in applied voltage: they initially react by producing a high current which tapers off over
time.
• A fully discharged capacitor initially acts as a short circuit (current with no voltage drop)
when faced with the sudden application of voltage. After charging fully to that level of
voltage, it acts as an open circuit (voltage drop with no current).
• In a resistor-capacitor charging circuit, capacitor voltage goes from nothing to full source
voltage while current goes from maximum to zero, both variables changing most rapidly
at first, approaching their final values slower and slower as time goes on.
16.3
Inductor transient response
Inductors have the exact opposite characteristics of capacitors. Whereas capacitors store en-
ergy in an electric field (produced by the voltage between two plates), inductors store energy
in a magnetic field (produced by the current through wire). Thus, while the stored energy in
a capacitor tries to maintain a constant voltage across its terminals, the stored energy in an
inductor tries to maintain a constant current through its windings. Because of this, inductors
16.3.
INDUCTOR TRANSIENT RESPONSE
503
oppose changes in current, and act precisely the opposite of capacitors, which oppose changes
in voltage. A fully discharged inductor (no magnetic field), having zero current through it, will
initially act as an open-circuit when attached to a source of voltage (as it tries to maintain
zero current), dropping maximum voltage across its leads. Over time, the inductor's current
rises to the maximum value allowed by the circuit, and the terminal voltage decreases cor-
respondingly. Once the inductor's terminal voltage has decreased to a minimum (zero for a
"perfect" inductor), the current will stay at a maximum level, and it will behave essentially as
a short-circuit.
Switch
15 V
1 Ω
1 H
R
L
When the switch is first closed, the voltage across the inductor will immediately jump to
battery voltage (acting as though it were an open-circuit) and decay down to zero over time
(eventually acting as though it were a short-circuit). Voltage across the inductor is determined
by calculating how much voltage is being dropped across R, given the current through the
inductor, and subtracting that voltage value from the battery to see what's left. When the
switch is first closed, the current is zero, then it increases over time until it is equal to the
battery voltage divided by the series resistance of 1 Ω. This behavior is precisely opposite
that of the series resistor-capacitor circuit, where current started at a maximum and capacitor
voltage at zero. Let's see how this works using real values:
0
Time (seconds)
1
2
3
4
5
6
7
8
9
10
0
2
4
6
8
10
12
14
16
Inductor voltage
Time
| Battery | Inductor
| Current
504
CHAPTER 16. RC AND L/R TIME CONSTANTS
voltage
0
15 V
15 V
0
0.5
15 V
9.098 V
1
15 V
5.518 V
2
15 V
2.030 V
3
15 V
0.747 V
4
15 V
0.275 V
5
15 V
0.101 V
6
15 V
10
15 V
Just as with the RC circuit, the inductor voltage's approach to 0 volts and the current's
approach to 15 amps over time is asymptotic. For all practical purposes, though, we can say
that the inductor voltage will eventually reach 0 volts and that the current will eventually
equal the maximum of 15 amps.
Again, we can use the SPICE circuit analysis program to chart this asymptotic decay of
inductor voltage and buildup of inductor current in a more graphical form (inductor current is
plotted in terms of voltage drop across the resistor, using the resistor as a shunt to measure
current):
inductor charging
v1 1 0 dc 15
r1 1 2 1
l1 2 0 1 ic=0
.tran .5 10 uic
.plot tran v(2,0) v(1,2)
.end
legend:
Inductor voltage
+: v(1,2) Inductor current
time
v(2)
5.000E+00
1.000E+01
1.500E+01
0.000E+00
5.000E-01
9.119E+00 .
16.4.
VOLTAGE AND CURRENT CALCULATIONS
505
1.000E+00
5.526E+00 .
1.500E+00
3.343E+00 .
2.000E+00
2.026E+00 .
2.500E+00
1.226E+00 .
3.000E+00
3.500E+00
4.000E+00
4.500E+00
5.000E+00
5.500E+00
6.000E+00
6.500E+00
7.000E+00
7.500E+00
8.000E+00
8.500E+00
9.000E+00
9.500E+00
1.000E+01
Notice how the voltage decreases (to the left of the plot) very quickly at first, then tapering
off as time goes on. Current also changes very quickly at first then levels off as time goes on,
but it is approaching maximum (right of scale) while voltage approaches minimum.
• REVIEW:
• A fully "discharged" inductor (no current through it) initially acts as an open circuit (volt-
age drop with no current) when faced with the sudden application of voltage. After "charg-
ing" fully to the final level of current, it acts as a short circuit (current with no voltage
drop).
• In a resistor-inductor "charging" circuit, inductor current goes from nothing to full value
while voltage goes from maximum to zero, both variables changing most rapidly at first,
approaching their final values slower and slower as time goes on.
16.4
Voltage and current calculations
There's a sure way to calculate any of the values in a reactive DC circuit over time.
The
first step is to identify the starting and final values for whatever quantity the capacitor or
inductor opposes change in; that is, whatever quantity the reactive component is trying to hold
constant. For capacitors, this quantity is voltage; for inductors, this quantity is current. When
the switch in a circuit is closed (or opened), the reactive component will attempt to maintain
that quantity at the same level as it was before the switch transition, so that value is to be
used for the "starting" value. The final value for this quantity is whatever that quantity will
be after an infinite amount of time. This can be determined by analyzing a capacitive circuit as
506
CHAPTER 16. RC AND L/R TIME CONSTANTS
though the capacitor was an open-circuit, and an inductive circuit as though the inductor was
a short-circuit, because that is what these components behave as when they've reached "full
charge," after an infinite amount of time.
The next step is to calculate the time constant of the circuit: the amount of time it takes
for voltage or current values to change approximately 63 percent from their starting values to
their final values in a transient situation. In a series RC circuit, the time constant is equal
to the total resistance in ohms multiplied by the total capacitance in farads. For a series L/R
circuit, it is the total inductance in henrys divided by the total resistance in ohms. In either
case, the time constant is expressed in units of seconds and symbolized by the Greek letter
"tau" (τ):
L
R
For resistor-capacitor circuits:
For resistor-inductor circuits:
The rise and fall of circuit values such as voltage and current in response to a transient is,
as was mentioned before, asymptotic. Being so, the values begin to rapidly change soon after
the transient and settle down over time. If plotted on a graph, the approach to the final values
of voltage and current form exponential curves.
As was stated before, one time constant is the amount of time it takes for any of these
values to change about 63 percent from their starting values to their (ultimate) final values.
For every time constant, these values move (approximately) 63 percent closer to their eventual
goal. The mathematical formula for determining the precise percentage is quite simple:
Percentage of change = 1 -
1
The letter e stands for Euler's constant, which is approximately 2.7182818. It is derived
from calculus techniques, after mathematically analyzing the asymptotic approach of the cir-
cuit values. After one time constant's worth of time, the percentage of change from starting
value to final value is:
1
e1
After two time constant's worth of time, the percentage of change from starting value to
final value is:
1
e2
After ten time constant's worth of time, the percentage is:
16.4.
VOLTAGE AND CURRENT CALCULATIONS
507
1
e10
The more time that passes since the transient application of voltage from the battery, the
larger the value of the denominator in the fraction, which makes for a smaller value for the
whole fraction, which makes for a grand total (1 minus the fraction) approaching 1, or 100
percent.
We can make a more universal formula out of this one for the determination of voltage and
current values in transient circuits, by multiplying this quantity by the difference between the
final and starting circuit values:
1
(Final-Start)
Universal Time Constant Formula
Where,
Value of calculated variable after infinite time
(its ultimate value)
Initial value of calculated variable
Euler's number ( 2.7182818)
Time in seconds
Time constant for circuit in seconds
Let's analyze the voltage rise on the series resistor-capacitor circuit shown at the beginning
of the chapter.
Switch
10 kΩ
100 μF
15 V
R
C
Note that we're choosing to analyze voltage because that is the quantity capacitors tend to
hold constant. Although the formula works quite well for current, the starting and final values
for current are actually derived from the capacitor's voltage, so calculating voltage is a more
direct method. The resistance is 10 kΩ, and the capacitance is 100 μF (microfarads). Since the
time constant (τ) for an RC circuit is the product of resistance and capacitance, we obtain a
value of 1 second:
508
CHAPTER 16. RC AND L/R TIME CONSTANTS
τ = (10 kΩ)(100 μF)
τ = 1 second
If the capacitor starts in a totally discharged state (0 volts), then we can use that value of
voltage for a "starting" value. The final value, of course, will be the battery voltage (15 volts).
Our universal formula for capacitor voltage in this circuit looks like this:
1
(Final-Start)
1
So, after 7.25 seconds of applying voltage through the closed switch, our capacitor voltage
will have increased by:
1
Change = 14.989 V
Change = (15 V - 0 V)(0.99929)
Since we started at a capacitor voltage of 0 volts, this increase of 14.989 volts means that
we have 14.989 volts after 7.25 seconds.
The same formula will work for determining current in that circuit, too. Since we know
that a discharged capacitor initially acts like a short-circuit, the starting current will be the
maximum amount possible: 15 volts (from the battery) divided by 10 kΩ(the only opposition
to current in the circuit at the beginning):
Starting current =
15 V
10 kΩ
Starting current = 1.5 mA
We also know that the final current will be zero, since the capacitor will eventually behave
as an open-circuit, meaning that eventually no electrons will flow in the circuit. Now that we
know both the starting and final current values, we can use our universal formula to determine
the current after 7.25 seconds of switch closure in the same RC circuit:
16.4.
VOLTAGE AND CURRENT CALCULATIONS
509
1
Change = (0 mA - 1.5 mA)(0.99929)
Note that the figure obtained for change is negative, not positive! This tells us that current
has decreased rather than increased with the passage of time. Since we started at a current
of 1.5 mA, this decrease (-1.4989 mA) means that we have 0.001065 mA (1.065 μA) after 7.25
seconds.
We could have also determined the circuit current at time=7.25 seconds by subtracting the
capacitor's voltage (14.989 volts) from the battery's voltage (15 volts) to obtain the voltage drop
across the 10 kΩresistor, then figuring current through the resistor (and the whole series
circuit) with Ohm's Law (I=E/R). Either way, we should obtain the same answer:
R
10 kΩ
I = 1.065 μA
The universal time constant formula also works well for analyzing inductive circuits. Let's
apply it to our example L/R circuit in the beginning of the chapter:
Switch
15 V
1 Ω
1 H
R
L
With an inductance of 1 henry and a series resistance of 1 Ω, our time constant is equal to
1 second:
L
R
1 H
1 Ω
1 second
510
CHAPTER 16. RC AND L/R TIME CONSTANTS
Because this is an inductive circuit, and we know that inductors oppose change in current,
we'll set up our time constant formula for starting and final values of current. If we start with
the switch in the open position, the current will be equal to zero, so zero is our starting current
value. After the switch has been left closed for a long time, the current will settle out to its final
value, equal to the source voltage divided by the total circuit resistance (I=E/R), or 15 amps in
the case of this circuit.
If we desired to determine the value of current at 3.5 seconds, we would apply the universal
time constant formula as such:
1
Change = 14.547 A
Change = (15 A - 0 A)(0.9698)
Given the fact that our starting current was zero, this leaves us at a circuit current of 14.547
amps at 3.5 seconds' time.
Determining voltage in an inductive circuit is best accomplished by first figuring circuit
current and then calculating voltage drops across resistances to find what's left to drop across
the inductor. With only one resistor in our example circuit (having a value of 1 Ω), this is rather
easy:
ER = (14.547 A)(1 Ω)
ER = 14.547 V
Subtracted from our battery voltage of 15 volts, this leaves 0.453 volts across the inductor
at time=3.5 seconds.
EL = 0.453 V
• REVIEW:
• Universal Time Constant Formula:
16.5.
WHY L/R AND NOT LR?
511
1
(Final-Start)
Universal Time Constant Formula
Where,
Value of calculated variable after infinite time
(its ultimate value)
Initial value of calculated variable
Euler's number ( 2.7182818)
Time in seconds
Time constant for circuit in seconds
• To analyze an RC or L/R circuit, follow these steps:
• (1): Determine the time constant for the circuit (RC or L/R).
• (2): Identify the quantity to be calculated (whatever quantity whose change is directly
opposed by the reactive component. For capacitors this is voltage; for inductors this is
current).
• (3): Determine the starting and final values for that quantity.
• (4): Plug all these values (Final, Start, time, time constant) into the universal time con-
stant formula and solve for change in quantity.
• (5): If the starting value was zero, then the actual value at the specified time is equal
to the calculated change given by the universal formula. If not, add the change to the
starting value to find out where you're at.
16.5
Why L/R and not LR?
It is often perplexing to new students of electronics why the time-constant calculation for an
inductive circuit is different from that of a capacitive circuit. For a resistor-capacitor circuit,
the time constant (in seconds) is calculated from the product (multiplication) of resistance
in ohms and capacitance in farads: τ=RC. However, for a resistor-inductor circuit, the time
constant is calculated from the quotient (division) of inductance in henrys over the resistance
This difference in calculation has a profound impact on the qualitative analysis of transient
circuit response. Resistor-capacitor circuits respond quicker with low resistance and slower
with high resistance; resistor-inductor circuits are just the opposite, responding quicker with
high resistance and slower with low resistance. While capacitive circuits seem to present no
intuitive trouble for the new student, inductive circuits tend to make less sense.
Key to the understanding of transient circuits is a firm grasp on the concept of energy
transfer and the electrical nature of it. Both capacitors and inductors have the ability to store
512
CHAPTER 16. RC AND L/R TIME CONSTANTS
quantities of energy, the capacitor storing energy in the medium of an electric field and the
inductor storing energy in the medium of a magnetic field. A capacitor's electrostatic energy
storage manifests itself in the tendency to maintain a constant voltage across the terminals.
An inductor's electromagnetic energy storage manifests itself in the tendency to maintain a
constant current through it.
Let's consider what happens to each of these reactive components in a condition of dis-
charge: that is, when energy is being released from the capacitor or inductor to be dissipated
in the form of heat by a resistor:
Capacitor and inductor discharge
Time
E
Time
I
heat
heat
Stored
energy
Dissipated
energy
energy
Dissipated
energy
Stored
In either case, heat dissipated by the resistor constitutes energy leaving the circuit, and
as a consequence the reactive component loses its store of energy over time, resulting in a
measurable decrease of either voltage (capacitor) or current (inductor) expressed on the graph.
The more power dissipated by the resistor, the faster this discharging action will occur, because
power is by definition the rate of energy transfer over time.
Therefore, a transient circuit's time constant will be dependent upon the resistance of the
circuit. Of course, it is also dependent upon the size (storage capacity) of the reactive compo-
nent, but since the relationship of resistance to time constant is the issue of this section, we'll
focus on the effects of resistance alone. A circuit's time constant will be less (faster discharging
rate) if the resistance value is such that it maximizes power dissipation (rate of energy trans-
fer into heat). For a capacitive circuit where stored energy manifests itself in the form of a
voltage, this means the resistor must have a low resistance value so as to maximize current
for any given amount of voltage (given voltage times high current equals high power). For
an inductive circuit where stored energy manifests itself in the form of a current, this means
the resistor must have a high resistance value so as to maximize voltage drop for any given
amount of current (given current times high voltage equals high power).
This may be analogously understood by considering capacitive and inductive energy storage
in mechanical terms. Capacitors, storing energy electrostatically, are reservoirs of potential
energy. Inductors, storing energy electromagnetically (electrodynamically), are reservoirs of
kinetic energy. In mechanical terms, potential energy can be illustrated by a suspended mass,
16.5.
WHY L/R AND NOT LR?
513
while kinetic energy can be illustrated by a moving mass. Consider the following illustration
as an analogy of a capacitor:
gravity
Cart
slope
Potential energy storage
and release
The cart, sitting at the top of a slope, possesses potential energy due to the influence of
gravity and its elevated position on the hill. If we consider the cart's braking system to be
analogous to the resistance of the system and the cart itself to be the capacitor, what resistance
value would facilitate rapid release of that potential energy? Minimum resistance (no brakes)
would diminish the cart's altitude quickest, of course! Without any braking action, the cart will
freely roll downhill, thus expending that potential energy as it loses height. With maximum
braking action (brakes firmly set), the cart will refuse to roll (or it will roll very slowly) and
it will hold its potential energy for a long period of time. Likewise, a capacitive circuit will
discharge rapidly if its resistance is low and discharge slowly if its resistance is high.
Now let's consider a mechanical analogy for an inductor, showing its stored energy in kinetic
form:
Cart
and release
Kinetic energy storage
This time the cart is on level ground, already moving. Its energy is kinetic (motion), not
potential (height). Once again if we consider the cart's braking system to be analogous to
circuit resistance and the cart itself to be the inductor, what resistance value would facilitate
rapid release of that kinetic energy? Maximum resistance (maximum braking action) would
slow it down quickest, of course! With maximum braking action, the cart will quickly grind to a
halt, thus expending its kinetic energy as it slows down. Without any braking action, the cart
will be free to roll on indefinitely (barring any other sources of friction like aerodynamic drag
and rolling resistance), and it will hold its kinetic energy for a long period of time. Likewise,
an inductive circuit will discharge rapidly if its resistance is high and discharge slowly if its
resistance is low.
Hopefully this explanation sheds more light on the subject of time constants and resistance,
and why the relationship between the two is opposite for capacitive and inductive circuits.
514
CHAPTER 16. RC AND L/R TIME CONSTANTS
16.6
Complex voltage and current calculations
There are circumstances when you may need to analyze a DC reactive circuit when the starting
values of voltage and current are not respective of a fully "discharged" state. In other words,
the capacitor might start at a partially-charged condition instead of starting at zero volts, and
an inductor might start with some amount of current already through it, instead of zero as we
have been assuming so far. Take this circuit as an example, starting with the switch open and
finishing with the switch in the closed position:
Switch
15 V
R1
R2
L
1 H
2 Ω
1 Ω
Since this is an inductive circuit, we'll start our analysis by determining the start and end
values for current. This step is vitally important when analyzing inductive circuits, as the
starting and ending voltage can only be known after the current has been determined! With
the switch open (starting condition), there is a total (series) resistance of 3 Ω, which limits the
final current in the circuit to 5 amps:
R
3 Ω
So, before the switch is even closed, we have a current through the inductor of 5 amps,
rather than starting from 0 amps as in the previous inductor example. With the switch closed
(the final condition), the 1 Ωresistor is shorted across (bypassed), which changes the circuit's
total resistance to 2 Ω. With the switch closed, the final value for current through the inductor
would then be:
R
2 Ω
16.7.
COMPLEX CIRCUITS
515
So, the inductor in this circuit has a starting current of 5 amps and an ending current of
7.5 amps. Since the "timing" will take place during the time that the switch is closed and R2
is shorted past, we need to calculate our time constant from L1 and R1: 1 Henry divided by 2
Ω, or τ = 1/2 second. With these values, we can calculate what will happen to the current over
time. The voltage across the inductor will be calculated by multiplying the current by 2 (to
arrive at the voltage across the 2 Ωresistor), then subtracting that from 15 volts to see what's
left. If you realize that the voltage across the inductor starts at 5 volts (when the switch is
first closed) and decays to 0 volts over time, you can also use these figures for starting/ending
values in the general formula and derive the same results:
1
Calculating current
1
Calculating voltage
. . . or . . .
Time
| Battery | Inductor
| Current
voltage
0
15 V
5 V
5 A
0.1
15 V
4.094 V
0.25
15 V
3.033 V
0.5
15 V
1.839 V
1
15 V
0.677 V
2
15 V
0.092 V
3
15 V
0.012 V
16.7
Complex circuits
What do we do if we come across a circuit more complex than the simple series configurations
we've seen so far? Take this circuit as an example:
516
CHAPTER 16. RC AND L/R TIME CONSTANTS
Switch
20 V
2 kΩ
R1
R2
500 Ω
R3
3 kΩ
C
100 μF
The simple time constant formula (τ=RC) is based on a simple series resistance connected to
the capacitor. For that matter, the time constant formula for an inductive circuit (τ=L/R) is also
based on the assumption of a simple series resistance. So, what can we do in a situation like
this, where resistors are connected in a series-parallel fashion with the capacitor (or inductor)?
The answer comes from our studies in network analysis. Thevenin's Theorem tells us that
we can reduce any linear circuit to an equivalent of one voltage source, one series resistance,
and a load component through a couple of simple steps. To apply Thevenin's Theorem to our
scenario here, we'll regard the reactive component (in the above example circuit, the capaci-
tor) as the load and remove it temporarily from the circuit to find the Thevenin voltage and
Thevenin resistance. Then, once we've determined the Thevenin equivalent circuit values,
we'll re-connect the capacitor and solve for values of voltage or current over time as we've been
doing so far.
After identifying the capacitor as the "load," we remove it from the circuit and solve for
voltage across the load terminals (assuming, of course, that the switch is closed):
Switch
(closed)
Thevenin
voltage
20 V
2 kΩ
500 Ω
R1
R2
R3
3 kΩ
1.8182 V
E
I
R
Volts
Amps
Ohms
R1
R2
R3
Total
2k
500
3k
5.5k
20
3.636m
3.636m
3.636m
3.636m
7.273
1.818
10.909
This step of the analysis tells us that the voltage across the load terminals (same as that
across resistor R2) will be 1.8182 volts with no load connected. With a little reflection, it should
be clear that this will be our final voltage across the capacitor, seeing as how a fully-charged
capacitor acts like an open circuit, drawing zero current. We will use this voltage value for our
Thevenin equivalent circuit source voltage.
16.7.
COMPLEX CIRCUITS
517
Now, to solve for our Thevenin resistance, we need to eliminate all power sources in the
original circuit and calculate resistance as seen from the load terminals:
Switch
(closed)
Thevenin
resistance
R1
R2
500 Ω
R3
3 kΩ
2 kΩ
454.545 Ω
RThevenin = 454.545 Ω
Re-drawing our circuit as a Thevenin equivalent, we get this:
Switch
EThevenin
1.8182 V
RThevenin
454.545 Ω
C
100 μF
Our time constant for this circuit will be equal to the Thevenin resistance times the capac-
itance (τ=RC). With the above values, we calculate:
τ = (454.545 Ω)(100 μF)
τ = 45.4545 milliseconds
Now, we can solve for voltage across the capacitor directly with our universal time constant
formula. Let's calculate for a value of 60 milliseconds. Because this is a capacitive formula,
we'll set our calculations up for voltage:
518
CHAPTER 16. RC AND L/R TIME CONSTANTS
Change = (Final - Start)
1
Change = (1.8182 V - 0 V)
1
e60m/45.4545m
Change = (1.8182 V)(0.73286)
Change = 1.3325 V
Again, because our starting value for capacitor voltage was assumed to be zero, the actual
voltage across the capacitor at 60 milliseconds is equal to the amount of voltage change from
zero, or 1.3325 volts.
We could go a step further and demonstrate the equivalence of the Thevenin RC circuit
and the original circuit through computer analysis. I will use the SPICE analysis program to
demonstrate this:
Comparison RC analysis
* first, the netlist for the original circuit:
v1 1 0 dc 20
r1 1 2 2k
r2 2 3 500
r3 3 0 3k
c1 2 3 100u ic=0
* then, the netlist for the thevenin equivalent:
v2 4 0 dc 1.818182
r4 4 5 454.545
c2 5 0 100u ic=0
* now, we analyze for a transient, sampling every .005 seconds
* over a time period of .37 seconds total, printing a list of
* values for voltage across the capacitor in the original
* circuit (between modes 2 and 3) and across the capacitor in
* the thevenin equivalent circuit (between nodes 5 and 0)
.tran .005 0.37 uic
.print tran v(2,3) v(5,0)
.end
time
v(2,3)
v(5)
0.000E+00
4.803E-06
4.803E-06
5.000E-03
1.890E-01
1.890E-01
1.000E-02
3.580E-01
3.580E-01
1.500E-02
5.082E-01
5.082E-01
2.000E-02
6.442E-01
6.442E-01
2.500E-02
7.689E-01
7.689E-01
3.000E-02
8.772E-01
8.772E-01
16.7.
COMPLEX CIRCUITS
519
3.500E-02
9.747E-01
9.747E-01
4.000E-02
1.064E+00
1.064E+00
4.500E-02
1.142E+00
1.142E+00
5.000E-02
1.212E+00
1.212E+00
5.500E-02
1.276E+00
1.276E+00
6.000E-02
1.333E+00
1.333E+00
6.500E-02
1.383E+00
1.383E+00
7.000E-02
1.429E+00
1.429E+00
7.500E-02
1.470E+00
1.470E+00
8.000E-02
1.505E+00
1.505E+00
8.500E-02
1.538E+00
1.538E+00
9.000E-02
1.568E+00
1.568E+00
9.500E-02
1.594E+00
1.594E+00
1.000E-01
1.617E+00
1.617E+00
1.050E-01
1.638E+00
1.638E+00
1.100E-01
1.657E+00
1.657E+00
1.150E-01
1.674E+00
1.674E+00
1.200E-01
1.689E+00
1.689E+00
1.250E-01
1.702E+00
1.702E+00
1.300E-01
1.714E+00
1.714E+00
1.350E-01
1.725E+00
1.725E+00
1.400E-01
1.735E+00
1.735E+00
1.450E-01
1.744E+00
1.744E+00
1.500E-01
1.752E+00
1.752E+00
1.550E-01
1.758E+00
1.758E+00
1.600E-01
1.765E+00
1.765E+00
1.650E-01
1.770E+00
1.770E+00
1.700E-01
1.775E+00
1.775E+00
1.750E-01
1.780E+00
1.780E+00
1.800E-01
1.784E+00
1.784E+00
1.850E-01
1.787E+00
1.787E+00
1.900E-01
1.791E+00
1.791E+00
1.950E-01
1.793E+00
1.793E+00
2.000E-01
1.796E+00
1.796E+00
2.050E-01
1.798E+00
1.798E+00
2.100E-01
1.800E+00
1.800E+00
2.150E-01
1.802E+00
1.802E+00
2.200E-01
1.804E+00
1.804E+00
2.250E-01
1.805E+00
1.805E+00
2.300E-01
1.807E+00
1.807E+00
2.350E-01
1.808E+00
1.808E+00
2.400E-01
1.809E+00
1.809E+00
2.450E-01
1.810E+00
1.810E+00
2.500E-01
1.811E+00
1.811E+00
2.550E-01
1.812E+00
1.812E+00
2.600E-01
1.812E+00
1.812E+00
520
CHAPTER 16. RC AND L/R TIME CONSTANTS
2.650E-01
1.813E+00
1.813E+00
2.700E-01
1.813E+00
1.813E+00
2.750E-01
1.814E+00
1.814E+00
2.800E-01
1.814E+00
1.814E+00
2.850E-01
1.815E+00
1.815E+00
2.900E-01
1.815E+00
1.815E+00
2.950E-01
1.815E+00
1.815E+00
3.000E-01
1.816E+00
1.816E+00
3.050E-01
1.816E+00
1.816E+00
3.100E-01
1.816E+00
1.816E+00
3.150E-01
1.816E+00
1.816E+00
3.200E-01
1.817E+00
1.817E+00
3.250E-01
1.817E+00
1.817E+00
3.300E-01
1.817E+00
1.817E+00
3.350E-01
1.817E+00
1.817E+00
3.400E-01
1.817E+00
1.817E+00
3.450E-01
1.817E+00
1.817E+00
3.500E-01
1.817E+00
1.817E+00
3.550E-01
1.817E+00
1.817E+00
3.600E-01
1.818E+00
1.818E+00
3.650E-01
1.818E+00
1.818E+00
3.700E-01
1.818E+00
1.818E+00
At every step along the way of the analysis, the capacitors in the two circuits (original circuit
versus Thevenin equivalent circuit) are at equal voltage, thus demonstrating the equivalence
of the two circuits.
• REVIEW:
• To analyze an RC or L/R circuit more complex than simple series, convert the circuit into
a Thevenin equivalent by treating the reactive component (capacitor or inductor) as the
"load" and reducing everything else to an equivalent circuit of one voltage source and one
series resistor. Then, analyze what happens over time with the universal time constant
formula.
16.8
Solving for unknown time
Sometimes it is necessary to determine the length of time that a reactive circuit will take to
reach a predetermined value. This is especially true in cases where we're designing an RC
or L/R circuit to perform a precise timing function. To calculate this, we need to modify our
"Universal time constant formula." The original formula looks like this:
1
(Final-Start)
16.8.
SOLVING FOR UNKNOWN TIME
521
However, we want to solve for time, not the amount of change. To do this, we algebraically
manipulate the formula so that time is all by itself on one side of the equal sign, with all the
rest on the other side:
Change
Change
Final-Start
Change
Final-Start
ln
ln
Change
Final - Start
The ln designation just to the right of the time constant term is the natural logarithm
function: the exact reverse of taking the power of e. In fact, the two functions (powers of e and
natural logarithms) can be related as such:
If ex = a, then the natural logarithm of a will give you x: the power that e must be was
raised to in order to produce a.
Let's see how this all works on a real example circuit. Taking the same resistor-capacitor
circuit from the beginning of the chapter, we can work "backwards" from previously determined
values of voltage to find how long it took to get there.
Switch
10 kΩ
100 μF
15 V
R
C
The time constant is still the same amount: 1 second (10 kΩtimes 100 μF), and the start-
ing/final values remain unchanged as well (EC = 0 volts starting and 15 volts final). According
to our chart at the beginning of the chapter, the capacitor would be charged to 12.970 volts at
the end of 2 seconds. Let's plug 12.970 volts in as the "Change" for our new formula and see if
we arrive at an answer of 2 seconds:
522
CHAPTER 16. RC AND L/R TIME CONSTANTS
12.970 V
t = 2 seconds
-(1 second)
t = (1 second)(2)
(ln 0.13534))
Indeed, we end up with a value of 2 seconds for the time it takes to go from 0 to 12.970 volts
across the capacitor. This variation of the universal time constant formula will work for all
capacitive and inductive circuits, both "charging" and "discharging," provided the proper values
of time constant, Start, Final, and Change are properly determined beforehand. Remember,
the most important step in solving these problems is the initial set-up. After that, its just a lot
of button-pushing on your calculator!
• REVIEW:
• To determine the time it takes for an RC or L/R circuit to reach a certain value of voltage
or current, you'll have to modify the universal time constant formula to solve for time
instead of change.
ln
Change
Final - Start
• The mathematical function for reversing an exponent of "e" is the natural logarithm (ln),
provided on any scientific calculator.
16.9
Contributors
Contributors to this chapter are listed in chronological order of their contributions, from most
recent to first. See Appendix 2 (Contributor List) for dates and contact information.
Jason Starck (June 2000): HTML document formatting, which led to a much better-
looking second edition.
Appendix A-1
ABOUT THIS BOOK
Purpose
They say that necessity is the mother of invention. At least in the case of this book, that adage
is true. As an industrial electronics instructor, I was forced to use a sub-standard textbook
during my first year of teaching. My students were daily frustrated with the many typograph-
ical errors and obscure explanations in this book, having spent much time at home struggling
to comprehend the material within. Worse yet were the many incorrect answers in the back of
the book to selected problems. Adding insult to injury was the $100+ price.
Contacting the publisher proved to be an exercise in futility. Even though the particular
text I was using had been in print and in popular use for a couple of years, they claimed my
complaint was the first they'd ever heard. My request to review the draft for the next edition
of their book was met with disinterest on their part, and I resolved to find an alternative text.
Finding a suitable alternative was more difficult than I had imagined. Sure, there were
plenty of texts in print, but the really good books seemed a bit too heavy on the math and the
less intimidating books omitted a lot of information I felt was important. Some of the best
books were out of print, and those that were still being printed were quite expensive.
It was out of frustration that I compiled Lessons in Electric Circuits from notes and ideas I
had been collecting for years. My primary goal was to put readable, high-quality information
into the hands of my students, but a secondary goal was to make the book as affordable as
possible. Over the years, I had experienced the benefit of receiving free instruction and encour-
agement in my pursuit of learning electronics from many people, including several teachers
of mine in elementary and high school. Their selfless assistance played a key role in my own
studies, paving the way for a rewarding career and fascinating hobby. If only I could extend
the gift of their help by giving to other people what they gave to me . . .
So, I decided to make the book freely available. More than that, I decided to make it "open,"
following the same development model used in the making of free software (most notably the
various UNIX utilities released by the Free Software Foundation, and the Linux operating
523
524
APPENDIX A-1.
ABOUT THIS BOOK
system, whose fame is growing even as I write). The goal was to copyright the text - so as to
protect my authorship - but expressly allow anyone to distribute and/or modify the text to suit
their own needs with a minimum of legal encumbrance. This willful and formal revoking of
standard distribution limitations under copyright is whimsically termed copyleft. Anyone can
"copyleft" their creative work simply by appending a notice to that effect on their work, but
several Licenses already exist, covering the fine legal points in great detail.
The first such License I applied to my work was the GPL - General Public License - of the
Free Software Foundation (GNU). The GPL, however, is intended to copyleft works of computer
software, and although its introductory language is broad enough to cover works of text, its
wording is not as clear as it could be for that application. When other, less specific copyleft
Licenses began appearing within the free software community, I chose one of them (the Design
Science License, or DSL) for my project. Once the author of the DSL said it was obsolete, I
moved on to Creative Commons (CC BY License).
In "copylefting" this text, I guaranteed that no instructor would be limited by a text insuffi-
cient for their needs, as I had been with error-ridden textbooks from major publishers. I'm sure
this book in its initial form will not satisfy everyone, but anyone has the freedom to change it,
leveraging my efforts to suit variant and individual requirements. For the beginning student
of electronics, learn what you can from this book, editing it as you feel necessary if you come
across a useful piece of information. Then, if you pass it on to someone else, you will be giving
them something better than what you received. For the instructor or electronics professional,
feel free to use this as a reference manual, adding or editing to your heart's content. The
only "catch" is this: if you plan to distribute your modified version of this text, you must give
credit where credit is due (to me, the original author, and anyone else whose modifications are
contained in your version), and you must ensure that whoever you give the text to is aware of
their freedom to similarly share and edit the text. The next chapter covers this process in more
detail.
It must be mentioned that although I strive to maintain technical accuracy in all of this
book's content, the subject matter is broad and harbors many potential dangers. Electricity
maims and kills without provocation, and deserves the utmost respect. I strongly encourage
experimentation on the part of the reader, but only with circuits powered by small batteries
where there is no risk of electric shock, fire, explosion, etc. High-power electric circuits should
be left to the care of trained professionals! The CC BY License clearly states that neither I nor
any contributors to this book bear any liability for what is done with its contents.
The use of SPICE
One of the best ways to learn how things work is to follow the inductive approach: to observe
specific instances of things working and derive general conclusions from those observations.
In science education, labwork is the traditionally accepted venue for this type of learning, al-
though in many cases labs are designed by educators to reinforce principles previously learned
through lecture or textbook reading, rather than to allow the student to learn on their own
through a truly exploratory process.
Having taught myself most of the electronics that I know, I appreciate the sense of frustra-
tion students may have in teaching themselves from books. Although electronic components
are typically inexpensive, not everyone has the means or opportunity to set up a laboratory
ACKNOWLEDGEMENTS
525
in their own homes, and when things go wrong there's no one to ask for help. Most textbooks
seem to approach the task of education from a deductive perspective: tell the student how
things are supposed to work, then apply those principles to specific instances that the student
may or may not be able to explore by themselves. The inductive approach, as useful as it is, is
hard to find in the pages of a book.
However, textbooks don't have to be this way. I discovered this when I started to learn a
computer program called SPICE. It is a text-based piece of software intended to model circuits
and provide analyses of voltage, current, frequency, etc. Although nothing is quite as good as
building real circuits to gain knowledge in electronics, computer simulation is an excellent al-
ternative. In learning how to use this powerful tool, I made a discovery: SPICE could be used
within a textbook to present circuit simulations to allow students to "observe" the phenomena
for themselves. This way, the readers could learn the concepts inductively (by interpreting
SPICE's output) as well as deductively (by interpreting my explanations). Furthermore, in
seeing SPICE used over and over again, they should be able to understand how to use it them-
selves, providing a perfectly safe means of experimentation on their own computers with circuit
simulations of their own design.
Another advantage to including computer analyses in a textbook is the empirical verifi-
cation it adds to the concepts presented. Without demonstrations, the reader is left to take
the author's statements on faith, trusting that what has been written is indeed accurate. The
problem with faith, of course, is that it is only as good as the authority in which it is placed and
the accuracy of interpretation through which it is understood. Authors, like all human beings,
are liable to err and/or communicate poorly. With demonstrations, however, the reader can
immediately see for themselves that what the author describes is indeed true. Demonstrations
also serve to clarify the meaning of the text with concrete examples.
SPICE is introduced in the book early on, and hopefully in a gentle enough way that it
doesn't create confusion. For those wishing to learn more, a chapter in the Reference volume
(volume V) contains an overview of SPICE with many example circuits. There may be more
flashy (graphic) circuit simulation programs in existence, but SPICE is free, a virtue comple-
menting the charitable philosophy of this book very nicely.
Acknowledgements
First, I wish to thank my wife, whose patience during those many and long evenings (and
weekends!) of typing has been extraordinary.
I also wish to thank those whose open-source software development efforts have made this
endeavor all the more affordable and pleasurable. The following is a list of various free com-
puter software used to make this book, and the respective programmers:
• GNU/Linux Operating System - Linus Torvalds, Richard Stallman, and a host of others
too numerous to mention.
• Vim text editor - Bram Moolenaar and others.
• Xcircuit drafting program - Tim Edwards.
• SPICE circuit simulation program - too many contributors to mention.
526
APPENDIX A-1.
ABOUT THIS BOOK
• Nutmeg post-processor program for SPICE - Wayne Christopher.
• TEX text processing system - Donald Knuth and others.
• Texinfo document formatting system - Free Software Foundation.
• LATEX document formatting system - Leslie Lamport and others.
• Gimp image manipulation program - too many contributors to mention.
Appreciation is also extended to Robert L. Boylestad, whose first edition of Introductory
Circuit Analysis taught me more about electric circuits than any other book. Other important
texts in my electronics studies include the 1939 edition of The "Radio" Handbook, Bernard
Grob's second edition of Introduction to Electronics I, and Forrest Mims' original Engineer's
Notebook.
Thanks to the staff of the Bellingham Antique Radio Museum, who were generous enough
to let me terrorize their establishment with my camera and flash unit. Similar thanks to the
Fluke Corporation in Everett, Washington, who not only let me photograph several pieces of
equipment in their primary standards laboratory, but proved their excellent hosting skills to a
large group of students and technical professionals one evening in November of 2001.
I wish to specifically thank Jeffrey Elkner and all those at Yorktown High School for being
willing to host my book as part of their Open Book Project, and to make the first effort in con-
tributing to its form and content. Thanks also to David Sweet (website: (http://www.andamooka.org))
and Ben Crowell (website: (http://www.lightandmatter.com)) for providing encourage-
ment, constructive criticism, and a wider audience for the online version of this book.
Thanks to Michael Stutz for drafting his Design Science License, to Richard Stallman for
pioneering the concept of copyleft, and to Creative Commons for the CC BY License.
Last but certainly not least, many thanks to my parents and those teachers of mine who
saw in me a desire to learn about electricity, and who kindled that flame into a passion for
discovery and intellectual adventure. I honor you by helping others as you have helped me.
Tony Kuphaldt, January 2002
"A candle loses nothing of its light when lighting another"
Kahlil Gibran
Appendix A-2
CONTRIBUTOR LIST
How to contribute to this book
As a copylefted work, this book is open to revision and expansion by any interested parties.
The only "catch" is that credit must be given where credit is due.
If you wish to cite portions of this book in a work of your own, you must follow the same
guidelines as for any other copyrighted work. Here is a sample for the CreativeCommons CC
BY License:
Subject to the terms and conditions of this Public License, the Licensor
hereby grants You a worldwide, royalty-free, non-sublicensable,
non-exclusive, irrevocable license to exercise the Licensed Rights in the
Licensed Material to:
A. reproduce and Share the Licensed Material, in whole or in part; and
B. produce, reproduce, and Share Adapted Material
No warranties are given. The license may not give you all of the
permissions necessary for your intended use. For example, other rights
such as publicity, privacy, or moral rights may limit how you use the
material.
If you wish to modify this book in any way, you should document the nature of those modifi-
cations in the "Credits" section along with your name, and ideally, information concerning how
you may be contacted. Again, the CC BY License:
You are free to:
(a) Share -- copy and redistribute the material in any medium
or format.
527
528
APPENDIX A-2.
CONTRIBUTOR LIST
(b)
Adapt -- remix, transform, and build upon the material
for any purpose, even commercially.
(c)
Attribution -- You must give appropriate credit, provide
a link to the license, and indicate if changes were made.
You may do so in any reasonable manner, but not in any
way that suggests the licensor endorses you or your use.
Given the complexities and security issues surrounding the maintenance of files comprising
this book, it is recommended that you submit any revisions or expansions to the original author
(Tony R. Kuphaldt). You are, of course, welcome to modify this book directly by editing your
own personal copy, but we would all stand to benefit from your contributions if your ideas were
incorporated into the online "master copy" where all the world can see it.
Credits
All entries arranged in alphabetical order of surname. Major contributions are listed by indi-
vidual name with some detail on the nature of the contribution(s), date, contact info, etc. Minor
contributions (typo corrections, etc.) are listed by name only for reasons of brevity. Please un-
derstand that when I classify a contribution as "minor," it is in no way inferior to the effort
or value of a "major" contribution, just smaller in the sense of less text changed. Any and all
contributions are gratefully accepted. I am indebted to all those who have given freely of their
own knowledge, time, and resources to make this a better book!
John Anhalt
• Date(s) of contribution(s): December 2008
• Nature of contribution: Updated lead-acid cell chemistry, Ch 11
• Contact at: jpa@anhalt.org
Benjamin Crowell, Ph.D.
• Date(s) of contribution(s): January 2001
• Nature of contribution: Suggestions on improving technical accuracy of electric field
and charge explanations in the first two chapters.
• Contact at: crowell01@lightandmatter.com
Dennis Crunkilton
• Date(s) of contribution(s): January 2006 to present
• Nature of contribution: Mini table of contents, all chapters except appedicies; html,
latex, ps, pdf; See Devel/tutorial.html; 01/2006.
CREDITS
529
• DC network analysis ch, Mesh current section, Mesh current by inspection, new mate-
rial.i DC network analysis ch, Node voltage method, new section.
• Ch3, Added AFCI paragraphs after GFCI, 10/09/2007.
• Contact at: liecibiblio(at)gmail.com
Tony R. Kuphaldt
• Date(s) of contribution(s): 1996 to present
• Nature of contribution: Original author.
• Contact at: liec0@lycos.com
Ron LaPlante
• Date(s) of contribution(s): October 1998
• Nature of contribution: Helped create the "table" concept for use in analysis of series
and parallel circuits.
Davy Van Nieuwenborgh
• Date(s) of contribution(s): October 2006
• Nature of contribution: DC network analysis ch, Mesh current section, supplied solu-
tion to mesh problem, pointed out error in text.
• Contact at: Theoretical Computer Science laboratory, Department of Computer
Science, Vrije Universiteit Brussel.
Ray A. Rayburn
• Date(s) of contribution(s): September 2009
• Nature of contribution: Nonapplicability of Maximum Power Transfer Theorem to Hi-
Fi audio amplifier.
• Contact at: http://forum.allaboutcircuits.com/member.php?u=54720
Jason Starck
• Date(s) of contribution(s): June 2000
• Nature of contribution: HTML formatting, some error corrections.
• Contact at: jstarck@yhslug.tux.org
530
APPENDIX A-2.
CONTRIBUTOR LIST
Warren Young
• Date(s) of contribution(s): August 2002
• Nature of contribution: Provided capacitor photographs for chapter 13.
someonesdad@allaboutcircuits.com
• Date(s) of contribution(s): November 2009
• Nature of contribution: Chapter 8, troublehooting tip end of Kelvin section.
Your name here
• Date(s) of contribution(s): Month and year of contribution
• Nature of contribution: Insert text here, describing how you contributed to the book.
• Contact at: my email@provider.net
Typo corrections and other "minor" contributions
• The students of Bellingham Technical College's Instrumentation program.
• anonymous (July 2007) Ch 1, remove :registers. Ch 5, s/figures something/figures is
something/. Ch 6 s/The current/The current. (September 2007) Ch 5, 8, 9, 10, 11, 12, 13,
15. Numerous typos, clarifications.
• Tony Armstrong (January 2003) Suggested diagram correction in "Series and Parallel
Combination Circuits" chapter.
• James Boorn (January 2001) Clarification on SPICE simulation.
• Dejan Budimir (January 2003) Clarification of Mesh Current method explanation.
• Sridhar Chitta, Assoc. Professor, Dept. of Instrumentation and Control Engg., Vignan
Institute of Technology and Science, Deshmukhi Village, Pochampally Mandal, Nalgonda
Distt, Andhra Pradesh, India (December 2005) Chapter 13: CAPACITORS, Clarification:
s/note the direction of current/note the direction of electron current/, 2-places
• Colin Creitz (May 2007) Chapters: several, s/it's/its.
• Larry Cramblett (September 2004) Typographical error correction in "Nonlinear con-
duction" section.
• Brad Drum (May 2006) Error correction in "Superconductivity" section, Chapter 12:
PHYSICS OF CONDUCTORS AND INSULATORS. Degrees are not used as a modifier
with kelvin(s), 3 changes.
• Jeff DeFreitas (March 2006)Improve appearance: replace "/" and "/" Chapters: A1, A2.
Type errors Chapter 3: /am injurious spark/an injurious spark/, /in the even/inthe event/
CREDITS
531
• Sean Donner (December 2004) Typographical error correction in "Voltage and current"
section, Chapter 1: BASIC CONCEPTS OF ELECTRICITY,(by a the/ by the) (current of
current/ of current).
(January 2005), Typographical error correction in "Fuses" section, Chapter 12: THE
PHYSICS OF CONDUCTORS AND INSULATORS (Neither fuses nor circuit breakers
were not designed to open / Neither fuses nor circuit breakers were designed to open).
(January 2005), Typographical error correction in "Factors Affecting Capacitance" sec-
tion, Chapter 13: CAPACITORS, (greater plate area gives greater capacitance; less plate
area gives less capacitance / greater plate area gives greater capacitance; less plate area
gives less capacitance); "Factors Affecting Capacitance" section, (thin layer if insula-
tion/thin layer of insulation).
(January 2005), Typographical error correction in "Practical Considerations" section, Chap-
ter 15: INDUCTORS, (there is not such thing / there is no such thing).
(January 2005), Typographical error correction in "Voltage and current calculations" sec-
tion, Chapter 16: RC AND L/R TIME CONSTANTS (voltage in current / voltage and
current).
• Manuel Duarte (August 2006): Ch: DC Metering Circuits ammeter images: 00163.eps,
00164.eps; Ch: RC and L/R Time Constants, simplified ln() equation images 10263.eps,
10264.eps, 10266.eps, 10276.eps.
• Aaron Forster (February 2003) Typographical error correction in "Physics of Conductors
and Insulators" chapter.
• Bill Heath (September-December 2002) Correction on illustration of atomic structure,
and corrections of several typographical errors.
• Stefan Kluehspies (June 2003): Corrected spelling error in Andrew Tannenbaum's
name.
• David M. St. Pierre (November 2007): Corrected spelling error in Andrew Tanenbaum's
name (from the title page of his book).
• Geoffrey Lessel,Thompsons Station, TN (June 2005): Corrected typo error in Ch 1 "If
this charge (static electricity) is stationary, and you won't realize-remove If; Ch 2 "Ohm's
Law also make intuitive sense if you apply if to the water-and-pipe analogy." s/if/it; Chap-
ter 2 "Ohm's Law is not very useful for analyzing the behavior of components like these
where resistance is varies with voltage and current." remove "is"; Ch 3 "which halts fibril-
lation and and gives the heart a chance to recover." double "and"; Ch 3 "To be safest, you
should follow this procedure is checking, using, and then checking your meter.... s/is/of.
• LouTheBlueGuru, allaboutcircuits.com, July 2005 Typographical errors, in Ch 6 "the
current through R1 is half:" s/half/twice; "current through R1 is still exactly twice that of
• Norm Meyrowitz , nkm, allaboutcircuits.com, July 2005 Typographical errors, in Ch 2.3
"where we don't know both voltage and resistance:" s/resistance/current
532
APPENDIX A-2.
CONTRIBUTOR LIST
• Don Stalkowski (June 2002) Technical help with PostScript-to-PDF file format conver-
sion.
• Joseph Teichman (June 2002) Suggestion and technical help regarding use of PNG
images instead of JPEG.
• Derek Terveer (June 2006) Typographical errors, several in Ch 1,2,3.
• Geoffrey Lessel (June 2005) Typographical error, s/It discovered/It was discovered/ in
Ch 1.
• Austin@allaboutcircuits.com (July 2007) Ch 2, units of mass, pound vs kilogram, near
"units of pound" s/pound/kilogram/.
• CATV@allaboutcircuits.com (April 2007) Telephone ring voltage error, Ch 3.
• line@allaboutcircuits.com (June 2005) Typographical error correction in Volumes 1,2,3,5,
various chapters ,(:s/visa-versa/vice versa/).
• rob843@allaboutcircuits.com (April 2007) Telephone ring voltage error, Ch 3.
• bigtwenty@allaboutcircuits.com (July 2007) Ch 4 near "different metric prefix", s/right
to left/left to right/.
• jut@allaboutcircuits.com (September 2007) Ch 13 near s/if were we to/if we were to/,
s/a capacitors/a capacitor.
• rxtxau@allaboutcircuits.com (October 2007) Ch 3, suggested, GFCI terminology, non-
US usage.
• Stacy Mckenna Seip (November 2007) Ch 3 s/on hand/one hand, Ch 4 s/weight/weigh,
Ch 8 s/weight/weigh, s/left their/left there, Ch 9 s/cannot spare/cannot afford/, Ch1 Clari-
fication, static electricity.
• Cory Benjamin (November 2007) Ch 3 s/on hand/one hand.
• Larry Weber (Feb 2008) Ch 3 s/on hand/one hand.
• trunks14@allaboutrcircuits.com (Feb 2008) Ch 15 s/of of/of .
• Greg Herrington (Feb 2008) Ch 1, Clarification: no neutron in hydrogen atom.
• mark44 (Feb 2008) Ch 1, s/naturaly/naturally/
• Unregistered@allaboutcircuits.com (February 2008) Ch 1, s/smokelsee/smokeless ,
• Timothy Unregistered@allaboutcircuits.com (Feb 2008) Changed default roman font
to newcent.
• Imranullah Syed (Feb 2008) Suggested centering of uncaptioned schematics.
• davidr@insyst ltd.com (april 2008) Ch 5, s/results/result 2plcs.
CREDITS
533
• Professor Thom@allaboutcircuits.com (Oct 2008) Ch 6, s/g/c near Ecd and near 00435.png,
2plcs.
• John Schwab (Dec 2008) Ch 1, Static Electricity, near Charles Coulomb: rearrangement
of text segments.
• Olivier Derewonko (Dec 2008) Ch 4 s/orientation a voltage/orientation of a/. Ch2 s/flow
• dor@allaboutcircuits.com (June 2009) Ch 1, s/nusiance/nuisance.
• rspuzio@allaboutcircuits.com (September 2009) Ch 8, s/logarithmic/nonlinear , 6-plcs.
• David Lewis@allaboutcircuits.com (September 2009) Ch 1, hide paragraph: Physical
dimension also impacts conductivity. . . etc.
• Walter Odington@allaboutcircuits.com (January 2010) Ch 3, s/hydration another/hydration
is another/ .
• tone b@allaboutcircuits.com (January 2010) Ch 6 , s/must were/were/ .
• Unregistered Guest@allaboutcircuits.com (July 2010) Ch 1 , s/is is/it is/ .
• Unregistered Guest@allaboutcircuits.com (July 2010) Ch 5 , added I2 to image 00090.png
• Unregistered Guest@allaboutcircuits.com (August 2010) Ch 1 , s/was one the/was
one of the/ .
• D. Crunkilton (June 2011) hi.latex, header file; updated link to openbookproject.net .
• Bob Arthur (Jan 2012) images: 00046.eps, 00047.eps,00048.eps 00362.eps, graph line
visibility fixed.
• vspriyan@allaboutcircuits.com (Jan 2013) Ch 10, Near: voltages divided by their
s/currents/resistances/ .
• Eugene Smirnoff (Jan 2013) Ch1, s/an hypothetical/a hypothetical/ . Ch 2 s/An his-
toric/A historic/ .
• Gulliveig@allaboutcircuits.com (Jan 2014) Ch4, s/significant digits/mantissa, s/1000/999/
• slidercrank@allaboutcircuits.com (Feb 2014) Ch6, s/both positive/both be positive/ .
• Skfir@allaboutcircuits.com (August 2015) Ch10, s/suppling/supplying/ .
• John Wang (Sept 2017) Ch2, s/points 1 and 4/points 1 and 6/, s/points 2 and 3/3 and 4/ .
• Stewart Todd Morgan (Feb 2020) Ch3, s+http://web.mit.edu/safety+https://ehs.mit.edu/workplace-
safety-program/electr
• DC (Sept 2017) Ch12, Chi3; Reformated various tables to html/latex.
534
APPENDIX A-2.
CONTRIBUTOR LIST
• DC (Nov 2021) ChA1, ChA2, ChA3; Replaced Design Science with Creative Commons CC
BY license
• Prafulla Giri (Nov 2022) Ch3, s/fat give provide/fat provide/; Ch13, s/ left it./left with./,
s/voltage reached/voltage reaches/, s/thusfar/thus far/, s/Temperature Coefficient Table/Relative
Permittivity Table; Ch15, s/Magnetic fields effect/Magnetic fields affect/
• Seth Cantrel (Feb 2023) Ch10; +E sub R1 instead of +E sub R
Appendix A-3
CC BY License
Creative Commons Attribution 4.0 International (CC
BY 4.0)
This is a human-readable summary of the full license code:
https://creativecommons.org/licenses/by/4.0/legalcode
This page was adapted from: https://creativecommons.org/licenses/by/4.0/
You are free to:
• Share - copy and redistribute the material in any medium or format
• Adapt - remix, transform, and build upon the material for any purpose, even commer-
cially.
The licensor cannot revoke these freedoms as long as you follow the license terms.
Under the following terms:
• Attribution - You must give appropriate credit, provide a link to the license, and indi-
cate if changes were made. You may do so in any reasonable manner, but not in any way
that suggests the licensor endorses you or your use.
• No additional restrictions - You may not apply legal terms or technological measures
that legally restrict others from doing anything the license permits./item¿
535
536
APPENDIX A-3.
CC BY LICENSE
Notices:
You do not have to comply with the license for elements of the material in the public domain or
where your use is permitted by an applicable exception or limitation.
No warranties are given. The license may not give you all of the permissions necessary for
your intended use. For example, other rights such as publicity, privacy, or moral rights may
limit how you use the material.
The full CC BY 4.0 license is available at Creative Commons:
https://creativecommons.org/licenses/by/4.0/legalcode
A simple explanation of your rights
This section was reused from Tony Kuphaldt, "Lessons In Industrial Instrumentation", Ap-
pendix B at:
https://ibiblio.org/kuphaldt/socratic/sinst/book/liii 0v2.pdf
This is an "open-source" textbook, which means the entirety of it is freely available for
public perusal, reproduction, distribution, and even modification. All digital "source" files com-
prising this textbook reside at the following website:
https://www.ibiblio.org/kuphaldt/electricCircuits
The Creative Commons Attribution license grants you (the recipient), as well as anyone who
might receive my work from you, the right to freely use it. This license also grants you (and
others) the right to modify my work, so long as you properly credit my original authorship and
declare these same rights (to my original work) for your own readers. My work is copyrighted
under United States law, but this license grants everyone else in the world certain freedoms
not customarily available under full copyright. This means no one needs to ask my permission,
or pay any royalties to me, in order to read, copy, distribute, publish, or otherwise use this
book.
If you choose to modify my work, you will have created what legal professionals refer to
as a derivative work. The Creative Commons license broadly groups derivative works under
the term adapted material. In simple terms, the fundamental restriction placed on you when
you do this is you must properly credit me for the portions of your adaptation that are my
original work. Otherwise, you may treat your adaptation the same way you would treat a
completely original work of your own.
This means you are legally permitted to enjoy full
copyright protection for your adaptation, up to and including exclusive rights of reproduction
and distribution. In other words, this license does not bind your derivative work under the
same terms and conditions I used to release my original work, although it does require you
notify your readers of this free source text and the License under which it is offered.
The practical upshot of this is you may modify my work and re-publish it as you would
any other book, with the full legal right to demand royalties, restrict distributions, etc. This
does not compromise the freedom of my original work, because that is still available to ev-
eryone under the terms and conditions of the Attribution license1. It does, however, protect
the investment(s) you make in creating the adaptation by allowing you to release the adapta-
tion under whatever terms you see fit (so long as those terms comply with current intellectual
property laws, of course).
A SIMPLE EXPLANATION OF YOUR RIGHTS
537
In summary, the following "legalese"2 is actually a very good thing for you, the reader of
my book. It grants you permission to do so much more with this text than what you would be
legally allowed to do with any other (traditionally copyrighted) book. It also opens the door to
open collaborative development, so it might grow into something far better than what I alone
could create.
1 You cannot pass my original work to anyone else under different terms or conditions than
the Attribution license. That is called sublicensing, and the Attribution license forbids it. In
fact, any re-distribution of my original work must come with a notice to the Attribution license,
so anyone receiving the book through you knows their rights.
2"legalese" at https://creativecommons.org/licenses/by/4.0/legalcode
538
APPENDIX A-3.
CC BY LICENSE
Index
10-50 milliamp signal, 309
3-15 PSI signal, 304
4-20 milliamp signal, 307
4-wire resistance measurement, 283
AC, 20, 79
Acid, 315
AFCI, 105
Algebraic sum, 181
Alligator clips, 284
Alternating current, 20, 79
Ammeter, 113, 253
Ammeter impact, 260
Ammeter, clamp-on, 263
Amp, 119
Amp-hour, 400
Ampacity, 417
Ampere (Amp), 36
Ampere (Amp), unit defined, 36
Amplified voltmeter, 249
Amplifier, 249
Analysis, Branch Current method, 332
Analysis, Loop Current method, 341
Analysis, Mesh Current method, 341
Analysis, network, 329
Analysis, node voltage, 357
Analysis, qualitative, 154, 216
Analysis, series-parallel, 200
Arc fault breaker, 105
Arc fault circuit interrupter, 105
Arm, Wheatstone bridge, 289
Asymptotic, 501, 504
Atom, 5
Atomic structure, 5, 391, 409
Atto, metric prefix, 123
AWG (American Wire Gauge), 414
B, symbol for magnetic flux density, 468
B&S (Brown and Sharpe), 414
Bank, battery, 406
Barrier strip, 161, 223
Battery, 18, 397
Battery capacity, 400
Battery charging, 396
Battery discharging, 395
Battery, charging, 407
Battery, Edison cell, 396
Battery, lead-acid cell, 394
Battery, sealed lead-acid cell, 407
Bifilar winding, 296
Bimetallic strip, 422
Block, terminal, 161
Bond, chemical, 392
Bond, covalent, 393
Bond, ionic, 393
Bonded strain gauge, 321
Branch Current analysis, 332
Breadboard, solderless, 156, 221
Breakdown, insulation, 54, 277, 437
Bridge circuit, 288
Bridge circuit, full-hyperpage, 326
Bridge circuit, half-hyperpage, 325
Bridge circuit, quarter-hyperpage, 322
Bridge, Kelvin Double, 289
Bridge, Wheatstone, 288
Busbar, 415
C, symbol for capacitance, 443
Cadmium cell, 402
Calculus, 444, 476, 485
Calculus, derivative function, 447, 489
Capacitance, 443
Capacitor, 439
Capacitor, electrolytic, 454
539
540
INDEX
Capacitor, tantalum, 458
Capacitor, variable, 452
Capacitors, nonpolarized, 457
Capacitors, polarized, 457
Capacitors, series and parallel, 453
Capacity, battery, 400
Cardio-Pulmonary Resuscitation, 97
Carrier, strain gauge, 321
Cathode Ray Tube, 239
Caustic, 315
Cell, 393, 397
Cell, chemical detection, 405
Cell, fuel, 403
Cell, mercury standard, 402
Cell, primary, 396
Cell, secondary, 396
Cell, solar, 404
Celsius (temperature scale), 144
Centi, metric prefix, 123
Centigrade, 144
Cgs, metric system, 468
Charge, early definition, 4
Charge, elementary, 7
Charge, modern definition, 7
Charge, negative, 7
Charge, positive, 7
Charging, battery, 396, 407
Charging, capacitor, 442
Charging, inductor, 483
Chip, 49
Choke, 484
Circuit, 12
Circuit breaker, 94, 422
Circuit, closed, 24
Circuit, equivalent, 370, 377, 384, 454, 498
Circuit, open, 24
Circuit, short, 23
Circuits, nonlinear, 369
Circular mil, 413
Closed circuit, 24
Cmil, 413
Common logarithm, 315
Compensation, thermocouple reference junc-
tion, 311
Computer simulation, 61
Condenser (or Condensor), 443
Conductance, 144
Conductivity, 8
Conductivity, earth, 99
Conductor, 8, 409
Conductor ampacity, 417
Conductor, ground-hyperpage, 103
Conductor, hot-hyperpage, 100, 425
Conductor, neutral-hyperpage, 100, 425
Continuity, 11, 111
Conventional flow, 30
Cooper pairs, 434
Coulomb, 5, 7, 36, 400
CPR, 97
CRT, 239
Current, 9, 14, 35
Current divider, 190
Current divider formula, 191
Current signal, 306
Current signal, 10-50 milliamp, 309
Current signal, 4-20 milliamp, 307
Current source, 306, 374
Current, alternating, 20
Current, direct, 20
Current, inrush, 424
Current, precise definition, 36, 43
D'Arsonval meter movement, 238
DC, 20, 79
Deca, metric prefix, 123
Deci, metric prefix, 123
Delta-Y conversion, 383
Derivative, calculus, 447, 489
Detector, 250
Detector, null, 250
Diamagnetism, 464
Dielectric, 443
Dielectric strength, 437
Digit, significant, 119
Diode, 31
Diode, zener, 403
Direct current, 20, 79
Discharging, battery, 395
Discharging, capacitor, 442
Discharging, inductor, 484
Disconnect switch, 93
Double insulation, 103
INDEX
541
Dynamic electricity, 9
Dynamometer meter movement, 295
e, symbol for Euler's constant, 506
e, symbol for instantaneous voltage, 36, 444,
476, 485
E, symbol for voltage, 36
Edison cell, 396
Effect, Meissner, 434
Effect, Peltier, 312
Effect, Seebeck, 310
Electric circuit, 12
Electric current, 9
Electric current, in a gas, 53
Electric field, 439
Electric motor, 466
Electric power, 42
Electric shock, 78
Electrically common points, 57, 80, 82
Electricity, static vs. dynamic-hyperpage, 9
Electrode, measurement-hyperpage, 316
Electrode, reference-hyperpage, 316
Electrolyte, 393
Electrolytic capacitor, 454
Electromagnetic induction, 475
Electromagnetism, 236, 466
Electromotive force, 36
Electron, 5, 391
Electron flow, 30
Electron gas, 410
Electron tube, 33, 55
Electron, free, 8, 410
Electrostatic meter movement, 238
Elementary charge, 7
Emergency response, 96
Energy, potential, 17
Engineering mode, calculator, 125
Equations, simultaneous, 331
Equations, systems of, 331
Equivalent circuit, 370, 377, 384, 454, 498
Esaki diode, 55
Euler's constant, 506
Exa, metric prefix, 123
Excitation voltage, bridge circuit, 327
Farad, 443
Fault, ground, 83
Femto, metric prefix, 123
Ferrite , 474
Ferromagnetism, 464
Fibrillation, cardiac, 79
Field flux, 440, 462, 481
Field force, 440, 462, 481
Field intensity, 467
Field, electric, 439
Field, magnetic, 481
Field-effect transistor, 249, 320
Flow, electron vs. conventional, 30
Flux density, 467
Force, electromotive, 36
Force, magnetomotive, 466
Four-wire resistance measurement, 283
Free electron, 8
Frequency, 87
Fuel cell, 403
Full-bridge circuit, 326
Fuse, 94, 419
Fusible link, 423
G, symbol for conductance, 144
Galvanometer, 236
Gauge, wire size, 414
Gauss, 468
GFCI, 98, 103, 105
Giga, metric prefix, 123
Gilbert, 468
Ground, 81
Ground fault, 83, 98, 103, 105
Ground Fault Current Interrupter, 98, 103, 105
Grounding, 81, 82, 84
H, symbol for magnetic field intensity, 468
Half-bridge circuit, 325
Hall-effect sensor, 263
Headphones, as sensitive null detector, 250
Hecto, metric prefix, 123
Henry, 484
Hertz, 87, 144
Hi-pot tester, 277
High voltage breakdown of insulation, 54, 277
Horsepower, 42
Hot wire, 100
542
INDEX
Hydrometer, 395
Hysteresis, 470
I, symbol for current, 36
i, symbol for instantaneous current, 36, 444,
485
IC, 49
Impedance, 381
Indicator, 302, 304
Inductance, 484
Inductance, mutual, 477, 498
Induction, electromagnetic, 475
Inductive reactance, 484
Inductor, 476, 481
Inductor, toroidal, 495
Inductors, series and parallel, 496
Inrush current, 424
Instantaneous value, 36, 444, 476, 485
Insulation breakdown, 54, 277
Insulation, wire, 92
Insulator, 8, 409, 436
Integrated circuit, 49
Ionization, 53
Ionization potential, 54
Iron-vane meter movement, 238
Josephson junction, 436
Joule, 37
Joule's Law, 46, 468
Jumper wire, 150
Junction, cold-hyperpage, 311
Junction, Josephson, 436
Junction, measurement-hyperpage, 310
Junction, reference-hyperpage, 311
KCL, 193, 195
kelvin (temperature scale), 434
Kelvin clips, 283, 284
Kelvin Double bridge, 289
Kelvin resistance measurement, 283
Kilo, metric prefix, 123
Kirchhoff's Current Law, 193
Kirchhoff's Voltage Law, 179
KVL, 179, 183
L, symbol for inductance, 484
Lead, test, 107
Lead-acid battery, 394
Leakage, capacitor, 442
Left-hand rule, 465
Lenz's Law, 487, 489
Lightning, 54
Linear, 52
Linearity, strain gauge bridge circuits, 327
Litmus strip, 315
Load, 50
Load cell, 327
Loading, voltmeter, 247
Lodestone, 461
Logarithm, common, 315
Logarithm, natural, 521
Loop Current analysis, 341
Magnet, permanent, 463
Magnetic field, 481
Magnetism, 461
Magnetite, 461
Magnetomotive force, 466
Maximum Power Transfer Theorem, 381
Maxwell, 468
Mega, metric prefix, 123
Megger, 271
Megohmmeter, 271
Meissner effect, 434
Mercury cell, 402
Mesh Current analysis, 341
Meter, 235
Meter movement, 236
Meter, null, 250
Metric system, 123
Metric system, cgs, 468
Metric system, mks, 468
Metric system, rmks, 468
Metric system, Systeme International (SI), 468
Metrology, 284
Mho, 144
Micro, metric prefix, 123
Mil, 412
Mil, circular, 413
Milli, metric prefix, 123
Milliamp, 87
INDEX
543
Millman's Theorem, 361, 379
Mks, metric system, 468
Molecule, 393
Motion, perpetual, 436
Motor, electric, 466
Movement, meter, 236
Multimeter, 106, 277
Multiplier, 242
Mutual inductance, 477, 498
MWG (Steel Music Wire Gauge), 414
Nano, metric prefix, 123
National Electrical Code, 417
Natural logarithm, 521
NEC, 417
Negative charge, 7
Negative resistance, 55
Netlist, SPICE, 65, 127, 138, 143
Network analysis, 329
Network theorem, 361
Neuron, 78
Neurotransmitter, 78
Neutral wire, 100
Neutron, 5, 391
Node number, SPICE, 62
Node voltage analysis, 357
Nonlinear, 52
Nonlinear circuit, 369
Nonpolarized, 31, 457
Normal magnetization curve, 470
Norton's Theorem, 373
Notation, scientific, 120
Nucleus, 6, 391, 409
Null detector, 250
Null meter, 250
Oersted, 468
Ohm, 36
Ohm's Law, 37, 468
Ohm's Law triangle-hyperpage, 39
Ohm's Law, correct context, 133, 139, 147
Ohm's Law, for magnetic circuits, 469
Ohm's Law, qualitative, 218
Ohm's Law, water analogy, 40
Ohmmeter, 264
Ohms per volt, 248
Open circuit, 24
Oscilloscope, 240
Over-unity machine, 436
Overcurrent protection, 95
P, symbol for power, 43
Parallel circuit rules, 144, 198
Parallel, definition of, 131
Paramagnetism, 464
Particle, 5, 391
PCB, 48, 159
pCO2, 315
Peltier effect, 312
Permanent magnet, 463
Permanent Magnet Moving Coil meter move-
ment, 236
Permeability, 467, 492
Permittivity, 443, 450
Perpetual motion machine, 436
Peta, metric prefix, 123
pH, 315
Photoelectric effect, 404
Physics, quantum, 409
Pico, metric prefix, 123
PMMC meter movement, 236
pO2, 315
Points, electrically common, 57, 80, 82
Polarity, 21
Polarity, voltage, 60
Polarized, 31, 457
Positive charge, 7
Potential energy, 17
Potential, ionization, 54
Potentiometer, 47, 178
Potentiometer, as voltage divider, 174
Potentiometer, precision, 178
Power calculations, 44
Power, electric, 42
Power, general definition, 512
Power, in series and parallel circuits, 146
Power, precise definition, 43
Primary cell, 396
Printed circuit board, 48, 159
Process variable, 302
Proton, 5, 119, 391
Proton, mass of, 119
544
INDEX
Q, symbol for electric charge, 36
Qualitative analysis, 154, 216
Quantum physics, 409
Quarter-bridge circuit, 322
R, symbol for resistance, 36
Radioactivity, 6
Ratio arm, Wheatstone bridge, 289
Re-drawing schematic diagrams, 208
Reactance, inductive, 484
Reactor, 484
Reference junction compensation, 311
Relay, 466
Reluctance, 467
Resistance, 23, 35
Resistance, internal to battery, 398
Resistance, negative, 55
Resistance, specific, 426
Resistance, temperature coefficient of, 431
Resistor, 46
Resistor, custom value, 296
Resistor, fixed, 47
Resistor, load, 50
Resistor, multiplier, 242
Resistor, potentiometer, 47
Resistor, shunt, 254
Resistor, swamping-hyperpage, 313
Resistor, variable, 47
Resistor, wire-wound, 296
Resolution, 301
Retentivity, 464
Rheostat arm, Wheatstone bridge, 289
Rmks, metric system, 468
RPM, 42
Rule, left-hand, 465
Rule, slide, 122
Rules, parallel circuits, 144, 198
Rules, series circuits, 138, 198
Saturation, 470
Scientific notation, 120
Secondary cell, 396
Seebeck effect, 310
Self-induction, 476
Semiconductor, 31, 410
Semiconductor diode, 31
Semiconductor fuse, 425
Semiconductor manufacture, 405
Sensitivity, ohms per volt, 248
Series circuit rules, 138, 198
Series, definition of, 131
Series-parallel analysis, 200
Shell, electron, 409
Shock hazard, AC, 79
Shock hazard, DC, 79
Shock, electric, 78
Short circuit, 23, 150
Shunt, 254
SI (Systeme International), metric system, 468
Siemens, 144
Signal, 301
Signal, 10-50 milliamp, 309
Signal, 3-15 PSI, 304
Signal, 4-20 milliamp, 307
Signal, analog, 301
Signal, current, 306
Signal, digital, 301
Signal, voltage, 304
Significant digit, 119
Simulation, computer, 61
Simultaneous equations, 331
Slide rule, 122
Slidewire, potentiometer, 174
Slow-blow fuse, 424
SMD, 49
Solar cell, 404
Soldering, 48, 159
Solderless breadboard, 156, 221
Source, current, 306, 374
Specific resistance, 426
Speedomax, 312
SPICE, 61, 126
SPICE netlist, 65, 127, 138
Standard cell, 402
Static electricity, 1, 7, 9
Strain gauge, 321
Strain gauge circuit linearity, 327
Strip, terminal, 161
Strong nuclear force, 6
Subscript, 50
Sum, algebraic, 181
Superconductivity, 9
INDEX
545
Superconductor, 434
Superfluidity, 434
Superposition Theorem, 364
Surface-mount device, 49
SWG (British Standard Wire Gauge), 414
Switch, 24
Switch, closed, 26
Switch, open, 26
Switch, safety disconnect, 93
System, metric, 123
Systems of equations, 331
Tachogenerator, 309
Tachometer, 309
Tantalum capacitor, 458
Temperature coefficient of resistance, 431
Temperature compensation, strain gauge, 324
Temperature, transition, 434
Tera, metric prefix, 123
Terminal block, 223
Terminal strip, 161, 223
Tesla, 468
Test lead, 107
Tetanus, 78
Tetrode, 55
Text editor, 62
Theorem, Maximum Power Transfer, 381
Theorem, Millman's, 361, 379
Theorem, network, 361
Theorem, Norton's, 373
Theorem, Superposition, 364
Theorem, Thevenin's, 369, 516
Thermocouple, 310
Thermopile, 312
Thevenin's Theorem, 369, 516
Time constant, 506
Time constant formula, 507
Toroidal core inductor, 495
Torque, 42
Trace, printed circuit board, 160
Transducer, 78
Transformer, 251, 477
Transient, 499
Transistor, 249, 277, 320, 405, 436
Transistor, field-effect, 249, 320
Transition temperature, 434
Transmitter, 302, 304
Troubleshooting, 149
Tube, vacuum, 249
Tube, vacuum or electron, 33
Tunnel diode, 55
Unit, ampere (amp), 36
Unit, Celsius, 144
Unit, centigrade, 144
Unit, cmil, 413
Unit, coulomb, 5, 7, 36, 400
Unit, farad, 443
Unit, gauss, 468
Unit, gilbert, 468
Unit, henry, 484
Unit, hertz, 87, 144
Unit, joule, 37
Unit, kelvin, 434
Unit, maxwell, 468
Unit, mho, 144
Unit, mil, 412
Unit, oersted, 468
Unit, ohm, 36
Unit, siemens, 144
Unit, tesla, 468
Unit, volt, 36
Unit, watt, 43
Unit, weber, 468
Universal time constant formula, 507
v, symbol for instantaneous voltage, 36, 444,
485
V, symbol for voltage, 36
Vacuum tube, 33, 249
Valence, 410
Variable capacitor, 452
Variable component, symbol modifier, 47
Varistor, 54, 469
Volt, 36
Volt, unit defined, 37
Voltage, 14, 35, 80
Voltage divider, 171
Voltage divider formula, 173
Voltage drop, 18
Voltage polarity, 21, 60, 182, 342
Voltage signal, 304
546
INDEX
Voltage, between common points, 59
Voltage, potential, 35
Voltage, precise definition, 17, 43
Voltage, sources, 18
Voltmeter, 110, 241
Voltmeter impact, 246
Voltmeter loading, 247
Voltmeter, amplified, 249
Voltmeter, null-balance, 250, 319
Voltmeter, potentiometric, 250, 319
VTVM, 249
Watt, 43
Wattmeter, 295
Weber, 468
Weston cell, 402
Weston meter movement, 238
Wheatstone bridge, 288, 322
Wheatstone bridge, unbalanced, 346
Winding, bifilar, 296
Wiper, potentiometer, 174
Wire, 10
Wire Gauge, 414
Wire, jumper, 150
Wire, solid and stranded, 411
Wire-wound resistor, 296
Wire-wrapping, 159
Work, 42
Working voltage, capacitor, 454
Y-Delta conversion, 383
Yocto, metric prefix, 123
Yotta, metric prefix, 123
Zener diode, 403
Zepto, metric prefix, 123
Zero energy state, 93
Zero, absolute, 434
Zero, live-hyperpage, 303
Zetta, metric prefix, 123
INDEX
547