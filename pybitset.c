#include <Python.h>
#include <structmember.h>

#define BIT_SET_SIZE(n) ((((n) + 7) & ~7) >> 3)

typedef struct
{
    PyObject_HEAD
    PyObject* buf; /* data buffer stored in a Python bytearray*/
    int size; /* the number of elements in the bitset */
    int nnz; /* the number of non-zero elements in the bitset */
} PyBitSet;

/* Manual indexing adjustment for custom methods */
static int
adjust_index(PyBitSet* self, int bit)
{
    return bit < 0 ? bit + self->size : bit;
}

static int
PyBitSet_traverse(PyBitSet* self, visitproc visit, void* arg)
{
    Py_VISIT(self->buf);
    return 0;
}

static int
PyBitSet_cleargc(PyBitSet* self)
{
    Py_CLEAR(self->buf);
    return 0;
}

static void
PyBitSet_dealloc(PyBitSet* self)
{
    PyBitSet_cleargc(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
PyBitSet_str(PyBitSet* obj)
{
    return PyUnicode_FromFormat("Bitset: size=%d, buf=%S, non-zero=%d",
                                obj->size, obj->buf, obj->nnz);
}

static int
PyBitSet_init(PyBitSet* self, PyObject* args, PyObject* kwds)
{
    int size, i;
    long init_val_int, elem;
    char* buf;
    PyObject* init_val = NULL, *iterator, *item;
    static char* kwlist[] = {"size", "init_val", NULL};

    /* accepts a size (int) and anoptional init_val */
    if(!PyArg_ParseTupleAndKeywords(args, kwds, "i|O", kwlist, &self->size, &init_val))
        return -1;

    size = BIT_SET_SIZE(self->size);
    buf = calloc(size, sizeof(char));

    if(PyLong_Check(init_val))
    {
        init_val_int = PyLong_AsLong(init_val);
        for(i = 0; i < size; ++i)
        {
            buf[i] = init_val_int & 0xFF;
            printf("buf[%d] = %d\n", i, buf[i] & 0xFF);
            init_val_int >>= 8;
        }
    }
    else if(PySequence_Check(init_val))
    {
        iterator = PyObject_GetIter(init_val);

        if(iterator == NULL)
        {
            free(buf);
            PyErr_SetString(PyExc_RuntimeError, "error while calling iter");
            return -1;
        }

        while((item = PyIter_Next(iterator)))
        {
            if(!PyLong_Check(item))
            {
                Py_DECREF(iterator);
                free(buf);
                PyErr_SetString(PyExc_ValueError, "sequence elements must be integers");
                return -1;
            }
            elem = PyLong_AsLong(item);
            if(elem >= self->size || elem < 0)
            {
                free(buf);
                PyErr_SetString(PyExc_ValueError, "values must be between zero (inclusive) and size (exclusive)");
                return -1;
            }
            ++self->nnz;
            buf[elem >> 3] |= 1 << (elem & 7);
            Py_DECREF(item);
        }

        Py_DECREF(iterator);

        if(PyErr_Occurred())
        {
            free(buf);
            PyErr_SetString(PyExc_RuntimeError, "an error occurred");
            return -1;
        }
    }
    else if(init_val)
    {
        free(buf);
        PyErr_SetString(PyExc_ValueError, "init_val must be an integer or a sequence of integers");
        return -1;
    }

    self->buf = PyByteArray_FromStringAndSize(buf, size);
    free(buf);
    if(self->buf == NULL)
    {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

PyDoc_STRVAR(size_doc, "The size of the bitset, i.e. the number of\
elements that the bitset can store.");

PyDoc_STRVAR(buf_doc, "Internal buffer of the bitset.");

PyDoc_STRVAR(nnz_doc, "Number of non-zero items in the bitset");

static PyMemberDef PyBitSet_members[] =
{
    {"size", T_INT, offsetof(PyBitSet, size), 0, size_doc},
    {"buf", T_OBJECT_EX, offsetof(PyBitSet, buf), 0, buf_doc},
    {"nnz", T_INT, offsetof(PyBitSet, nnz), 0, nnz_doc},
    {NULL}  /* Sentinel */
};


static PyObject*
PyBitSet_change_one(PyBitSet* self, Py_ssize_t bit, int set)
{
    int mask;
    Py_buffer view;
    uint8_t* buf;

    if(bit >= self->size || bit < 0)
    {
        PyErr_SetString(PyExc_IndexError, "invalid index");
        return NULL;
    }

    if(PyObject_GetBuffer(self->buf, &view, PyBUF_WRITABLE) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }
    buf = (uint8_t*) view.buf;
    mask = 1 << (bit & 7);
    if(set)
    {
        if(!(buf[bit >> 3] & mask))
            ++self->nnz;
        buf[bit >> 3] |= mask;
    }
    else
    {
        if(buf[bit >> 3] & mask)
            --self->nnz;
        buf[bit >> 3] &= ~mask;
    }
    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

static PyObject*
PyBitSet_to_int(PyBitSet* self)
{
    unsigned long val = 0;
    int size = BIT_SET_SIZE(self->size), i;
    Py_buffer view;
    uint8_t* buf;
    if(PyObject_GetBuffer(self->buf, &view, PyBUF_WRITABLE) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }
    buf = (uint8_t*) view.buf;
    for(i = 0; i < size; ++i)
        val += ((unsigned long)(buf[i] & 0xFF)) << (8 * i);

    PyBuffer_Release(&view);
    return PyLong_FromLong(val);
}

static int
PyBitSet_SetItem(PyBitSet* o, Py_ssize_t i, PyObject* v)
{
    Py_ssize_t val;
    PyObject* tmp;
    if(!PyNumber_Check(v))
    {
        PyErr_SetString(PyExc_ValueError, "__setitem__ accepts only numeric values");
        return -1;
    }
    val = PyNumber_AsSsize_t(v, NULL);
    if(val != 0)
        val = 1;
    tmp = PyBitSet_change_one(o, i, val);
    if(tmp == NULL)
        return -1;
    else
    {
        Py_DECREF(tmp);
        return 0;
    }
}

static PyObject*
PyBitSet_flip_one(PyBitSet* self, PyObject* args)
{
    int bit, set, mask;
    Py_buffer view;
    uint8_t* buf;

    if(!PyArg_ParseTuple(args, "i", &bit))
        return NULL;

    if(bit >= self->size || bit < -self->size)
    {
        PyErr_SetString(PyExc_IndexError, "invalid index");
        return NULL;
    }
    if(PyObject_GetBuffer(self->buf, &view, PyBUF_WRITABLE) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }

    bit = adjust_index(self, bit);
    buf = (uint8_t*) view.buf;
    set = (buf[bit >> 3] >> (bit & 7)) & 1;
    mask = 1 << (bit & 7);

    if(!set)
    {
        buf[bit >> 3] |= mask;
        ++self->nnz;
    }
    else
    {
        buf[bit >> 3] &= ~mask;
        --self->nnz;
    }

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(flip_one_doc, "Flips the bit at specified position.");

static PyObject*
PyBitSet_flip_all(PyBitSet* self)
{
    int size = BIT_SET_SIZE(self->size), i, acc;
    Py_buffer view;
    unsigned char* buf;
    if(PyObject_GetBuffer(self->buf, &view, PyBUF_WRITABLE) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }
    buf = (uint8_t*) view.buf;
    self->nnz = 0;
    for(i = 0; i < size; ++i)
    {
        buf[i] = ~buf[i];
        if(i == size - 1)
        {
            acc = 8 - (size * 8 - self->size);
            buf[size - 1] &= (1 << acc) - 1;
        }
        acc = buf[i] & 0xFF;
        while(acc)
        {
            self->nnz += acc & 1;
            acc >>= 1;
        }
    }

    PyBuffer_Release(&view);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(flip_all_doc, "Flips all the bits.");

static PyObject*
PyBitSet_to_bin_str(PyBitSet* self)
{
    char* str;
    int i = 0, j = 0, size = BIT_SET_SIZE(self->size), index, val;
    Py_buffer view;
    uint8_t* buf;
    if(PyObject_GetBuffer(self->buf, &view, 0) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }

    buf = (uint8_t*)view.buf;
    str = calloc(self->size + 1, sizeof(char));

    for(; i < size; ++i)
    {
        val = buf[i];
        for(j = 0; j < 8; ++j)
        {
            index = (i << 3) + j;
            str[self->size - index - 1] = (val >> j) & 1 ? '1' : '0';
            if(i == size - 1 && index > self->size - 1)
                goto exit_loop;
        }
    }
exit_loop:
    return PyUnicode_FromString(str);
}

PyDoc_STRVAR(to_bin_str_doc, "Returns a binary string representing the bitset.");

static PyObject*
PyBitSet_GetItem(PyBitSet* self, Py_ssize_t bit)
{
    int val;
    Py_buffer view;
    uint8_t* buf;

    if(bit >= self->size || bit < 0)
    {
        PyErr_SetString(PyExc_IndexError, "invalid index");
        return NULL;
    }

    if(PyObject_GetBuffer(self->buf, &view, 0) < 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "buffer read error");
        return NULL;
    }
    buf = (uint8_t*) view.buf;
    val = buf[bit >> 3];
    val >>= (bit & 7);
    PyBuffer_Release(&view);
    return PyLong_FromLong(val & 1);
}

static Py_ssize_t
PyBitSet_Length(PyBitSet* self)
{
    return self->size;
}

static int
PyBitSet_Contains(PyBitSet* o, PyObject* value)
{
    int val;
    PyObject* tmp;
    if(!PyLong_Check(value))
    {
        PyErr_SetString(PyExc_ValueError, "__contains__ accepts only numeric values");
        return -1;
    }
    val = PyNumber_AsSsize_t(value, NULL);
    tmp = PyBitSet_GetItem(o, val);
    if(tmp == NULL)
        return -1;
    val = PyLong_AsLong(tmp);
    Py_DECREF(tmp);
    return val;
}

static PyMethodDef PyBitSet_methods[] =
{
    {"flip_one", (PyCFunction)PyBitSet_flip_one, METH_VARARGS, flip_one_doc},
    {"flip_all", (PyCFunction)PyBitSet_flip_all, METH_NOARGS, flip_all_doc},
    {"to_bin_str", (PyCFunction)PyBitSet_to_bin_str, METH_NOARGS, to_bin_str_doc},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PyBitSet_num =
{
    0,       /*nb_add*/
    0,       /*nb_subtract*/
    0,       /*nb_multiply*/
    0,                   /*nb_remainder*/
    0,                /*nb_divmod*/
    0,                   /*nb_power*/
    0,        /*nb_negative*/
    0,       /*tp_positive*/
    0,        /*tp_absolute*/
    0,         /*tp_bool*/
    0,     /*nb_invert*/
    0,                /*nb_lshift*/
    0,    /*nb_rshift*/
    0,                   /*nb_and*/
    0,                   /*nb_xor*/
    0,                    /*nb_or*/
    (unaryfunc) PyBitSet_to_int,                  /*nb_int*/
    0,                          /*nb_reserved*/
    0,                 /*nb_float*/
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                   /* nb_floor_divide */
    0,           /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    (unaryfunc) PyBitSet_to_int,                  /* nb_index */
};

static PySequenceMethods PyBitSet_seq =
{
    (lenfunc)PyBitSet_Length,                       /* sq_length */
    0,                    /* sq_concat */
    0,                  /* sq_repeat */
    (ssizeargfunc)PyBitSet_GetItem,                    /* sq_item */
    0,                                          /* sq_slice */
    (ssizeobjargproc)PyBitSet_SetItem,             /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    (objobjproc)PyBitSet_Contains,                  /* sq_contains */
    0,            /* sq_inplace_concat */
    0,          /* sq_inplace_repeat */
};

static PyTypeObject PyBitSetType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyBitSet.PyBitSet",       /* tp_name */
    sizeof(PyBitSet),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyBitSet_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    &PyBitSet_num,             /* tp_as_number */
    &PyBitSet_seq,             /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    (reprfunc) PyBitSet_str,   /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_GC,   /* tp_flags */
    "PyBitSet objects",           /* tp_doc */
    (traverseproc)PyBitSet_traverse,   /* tp_traverse */
    (inquiry)PyBitSet_cleargc,           /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    PyBitSet_methods,             /* tp_methods */
    PyBitSet_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyBitSet_init,      /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,                 /* tp_new */
};

static PyModuleDef pyBitSetmodule =
{
    PyModuleDef_HEAD_INIT,
    "pyBitSet",
    "PyBitSet example",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_pyBitSet(void)
{
    PyObject* m;

    if(PyType_Ready(&PyBitSetType) < 0)
        return NULL;

    m = PyModule_Create(&pyBitSetmodule);
    if(m == NULL)
        return NULL;

    Py_INCREF(&PyBitSetType);
    PyModule_AddObject(m, "PyBitSet", (PyObject*)&PyBitSetType);
    return m;
}
