﻿module AST

open System

type Identifier = string

type Type =
    | TypeName of string
    | ImplicitType

type Literal =
    | Int of int
    | Float of float
    | Char of char
    | String of string
    | Bool of bool
    | Identifier of string

type Wildcard = obj

type Expr =
    | Literal of Literal
    | ListValue of Expr list
    | TupleValue of Expr list
    | ToExpr of Expr * Expr
    | Variable of Identifier
    | Invoke of Literal * Expr list
    | InfixOp of Expr * string * Expr
    | Dot of Expr * string * Expr
    | PrefixOp of string * Expr
    | PostfixOp of Expr * string
    | TernaryOp of TernaryCondition * TernaryTrue * TernaryFalse
    | TypeConstructor of Identifier * Expr list
    | Constructor of Identifier * Param list
    | Expression of Expr
    | MatchExpr of Case list
    | Value of Expr
    | Lambda of String list * Expr
    | Extern of string * string * Expr list
and TernaryCondition = Condition of Expr
and TernaryTrue = IfTrue of Expr
and TernaryFalse = IfFalse of Expr
and Case = 
    | Case of Expr * Expr
    | Wildcard of Wildcard
and Param = Param of Expr

type Define = Define of Identifier * Type
type Init = Assign of Identifier * Expr

type DefaultValueArg =
    | DefaultValueArg of Expr
    | NoDefaultValueArg

type Arg = Define * DefaultValueArg

type Arguments = Arg list

type Parameters = Expr list

type To = To of Expr
type Step = Step of Expr
type Each = Each of Expr
type In = In of Expr

type ArgConstructor = ArgFieldConstructor of Define * DefaultValueArg

type Constructor = Identifier * ArgConstructor list

type Statement =
    | VarDeclr of Identifier * Type * Expr
    | LetDeclr of string * Type * Expr
    | LetFuncDeclr of Identifier * Arguments * Type * Expr
    | FuncDefinition of Identifier * Arguments * Type * Block
    | FuncInvocation of Identifier * Parameters
    | Assignment of Init
    | AnonymousExpression of Expr
    | If of Expr * Block
    | IfElse of Expr * Block * Block
    | MatchStmt of Expr * Case
    | For of Init * To * Block
    | ForStep of Init * To * Step * Block
    | ForEach of Define * In * Block
    | While of Expr * Block
    | DoWhile of Block * Expr
    | Throw of Expr
    | Catch of Define * Block
    | Try of Block
    | Continue
    | Return of Expr
    | TypeAsAlias of Type * Type
    | TypeAsStruct of Constructor
    | TypeAsClass of Constructor * Statement list //* Member list
    | Include of string
    | Import of string
and Block = Statement list
and TypeMemberAccess =
    | Public
    | Private
and Member = TypeMemberAccess * Statement

type Program = Program of Statement list
