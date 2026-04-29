We are working on a project to test and develop my harness engineering skills.

First, I would like you to read this article: https://openai.com/index/harness-engineering/

The goal of this project is to build a full agentic first pipeline that does the design, coding, review, and submission of code.

Game Information and Requirements
1) We are making a game of chess
2) All pieces and moves should work by their known rules
3) The game should be able to be restart after a winner is decided
4) Check and Checkmate should be displayed to the user

Application Requirements and Constraints
1) This will be a Windows app only
2) The game requires graphics, and should be done via 2D software rendering using the Windows API
3) The game will only need to be played with a mouse. Click and drag to move pieces
4) The game will only be played on a single machine, so two players switching off with the mouse

Build System and Source Control
1) Building the code should be done by a simple .ps1 script that invokes the cl.exe
2) You are in a github repository and will be using github for source control
3) You can use the tools available to you in github, like the Issues feature to track work

Code Style and Architecture
1) This is a C++ application
2) Do NOT use object oriented programming: Use C-style APIs for boundaries, structs with public variables, etc
3) Avoid dynamic memory allocation during runtime. We can allocate a block of memory up front and use that
4) The memory should be divided up into separate arenas for different systems
5) You will not be using any 3rd party libraries. Avoid the C standard library as well

Testing
1) Testing should happen in the application. It should be a first class citizen of the application, we are not using a testing harness or a separate application to do unit testing. The application should be able to start up in a testing mode that runs through a suite of tests that you will be creating and maintaining. 

This is all I can think of right now. Given these requirements and the above article, we are going to create the initial commit now.
1) Ask me any questions that will help you make the correct choices. If you are uncertain of something, ask!
2) This initial commit should not have code. We are setting up the repo for an agentic first workflow